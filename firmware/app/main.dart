import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';
import 'dart:async';
import 'dart:convert';
import 'package:pdf/pdf.dart';
import 'package:pdf/widgets.dart' as pw;
import 'package:printing/printing.dart';
import 'package:path_provider/path_provider.dart';
import 'dart:io';
import 'package:intl/intl.dart';
import 'package:collection/collection.dart';

void main() {
  runApp(const HealthMonitorApp());
}

class HealthMonitorApp extends StatelessWidget {
  const HealthMonitorApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Health Monitor Pro',
      theme: ThemeData(
        primarySwatch: Colors.blue,
        useMaterial3: true,
        visualDensity: VisualDensity.adaptivePlatformDensity,
      ),
      home: const HealthMonitorPage(),
    );
  }
}

class HealthDataPoint {
  final DateTime timestamp;
  final double heartRate;
  final double temperature;
  final int gsr;
  final String mood;
  final bool isCalibration;
  final int batteryPercentage;

  HealthDataPoint({
    required this.timestamp,
    required this.heartRate,
    required this.temperature,
    required this.gsr,
    required this.mood,
    required this.isCalibration,
    required this.batteryPercentage,
  });

  Map<String, dynamic> toMap() {
    return {
      'timestamp': timestamp.toIso8601String(),
      'heartRate': heartRate,
      'temperature': temperature,
      'gsr': gsr,
      'mood': mood,
      'isCalibration': isCalibration,
      'batteryPercentage': batteryPercentage, 
    };
  }

  factory HealthDataPoint.fromMap(Map<String, dynamic> map) {
    return HealthDataPoint(
      timestamp: DateTime.parse(map['timestamp']),
      heartRate: map['heartRate'],
      temperature: map['temperature'],
      gsr: map['gsr'],
      mood: map['mood'],
      isCalibration: map['isCalibration'] ?? false,
      batteryPercentage: map['batteryPercentage'] ?? 0, 
    );
  }

  static List<HealthDataPoint> filterByDate(List<HealthDataPoint> data, DateTime targetDate) {
    return data.where((point) {
      return point.timestamp.year == targetDate.year &&
            point.timestamp.month == targetDate.month &&
            point.timestamp.day == targetDate.day;
    }).toList();
  }
}

class _HealthMonitorPageState extends State<HealthMonitorPage> {
  BluetoothDevice? _connectedDevice;
  BluetoothCharacteristic? _targetCharacteristic;
  bool _isConnected = false;
  bool _isScanning = false;
  StreamSubscription<List<int>>? _characteristicSubscription;
  StreamSubscription<List<ScanResult>>? _scanSubscription;

  // Sensor Data
  double _heartRate = 0;
  double _temperature = 0;
  int _gsr = 0;
  String _mood = "Waiting for connection...";
  double _progress = 0;
  bool _fingerDetected = false;
  String _statusMessage = "Searching for device...";
  int _batteryPercentage = 0;


  // Data Storage with 30-day retention
  final List<HealthDataPoint> _healthData = [];
  static const int _maxDataAgeDays = 30;
  Timer? _dataCollectionTimer;

  // BLE Configuration
  final String _targetDeviceName = "Health_Monitor_BLE";
  final String _serviceUUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
  final String _characteristicUUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";

  @override
  void initState() {
    super.initState();
    _initBluetooth();
    _loadStoredData();

  }

  Color _getBatteryColor(int percentage) {
    if (percentage > 60) return Colors.green;
    if (percentage > 30) return Colors.orange;
    if (percentage > 15) return Colors.red;
    return Colors.grey;
  }

  Future<void> _loadStoredData() async {
    try {
      final directory = await getApplicationDocumentsDirectory();
      final file = File('${directory.path}/health_data.json');
      
      if (await file.exists()) {
        final contents = await file.readAsString();
        final List<dynamic> jsonList = jsonDecode(contents);
        setState(() {
          _healthData.addAll(jsonList.map((item) => HealthDataPoint.fromMap(item)).toList());
          _purgeOldData();
        });
      }
    } catch (e) {
      debugPrint("Error loading data: $e");
    }
  }

  Future<void> _saveData() async {
    try {
      final directory = await getApplicationDocumentsDirectory();
      final file = File('${directory.path}/health_data.json');
      
      final jsonList = _healthData.map((point) => point.toMap()).toList();
      await file.writeAsString(jsonEncode(jsonList));
    } catch (e) {
      debugPrint("Error saving data: $e");
    }
  }

  void _purgeOldData() {
    final cutoffDate = DateTime.now().subtract(const Duration(days: _maxDataAgeDays));
    _healthData.removeWhere((point) => point.timestamp.isBefore(cutoffDate));
  }

  void _addDataPoint(HealthDataPoint newPoint) {
    setState(() {
      _purgeOldData();
      _healthData.add(newPoint);
    });
    _saveData();
  }

  Future<void> _clearAllData() async {
  final confirmed = await showDialog<bool>(
    context: context,
    builder: (BuildContext context) {
      return AlertDialog(
        title: const Text('Clear All Data'),
        content: const Text('Are you sure you want to delete all stored health data? This action cannot be undone.'),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(false),
            child: const Text('Cancel'),
          ),
          TextButton(
            onPressed: () => Navigator.of(context).pop(true),
            style: TextButton.styleFrom(foregroundColor: Colors.red),
            child: const Text('Clear All'),
          ),
        ],
      );
    },
  );

  if (confirmed == true) {
    setState(() {
      _healthData.clear();
    });
    
    // Clear the stored file
    try {
      final directory = await getApplicationDocumentsDirectory();
      final file = File('${directory.path}/health_data.json');
      if (await file.exists()) {
        await file.delete();
      }
    } catch (e) {
      debugPrint("Error clearing stored data: $e");
    }

    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(
        content: Text("All health data cleared successfully"),
        backgroundColor: Colors.green,
      ),
    );
  }
}
  Future<void> _initBluetooth() async {
    await Permission.bluetoothScan.request();
    await Permission.bluetoothConnect.request();
    await Permission.locationWhenInUse.request();

    _scanSubscription = FlutterBluePlus.scanResults.listen((results) {
      for (ScanResult result in results) {
        if (result.device.platformName == _targetDeviceName && !_isConnected) {
          _connectToDevice(result.device);
          break;
        }
      }
    });

    _startAutoScan();
  }

  void _startAutoScan() {
    if (_isConnected) return;
    
    setState(() {
      _isScanning = true;
      _statusMessage = "Searching for $_targetDeviceName...";
    });

    FlutterBluePlus.startScan(
      timeout: const Duration(seconds: 4),
      androidUsesFineLocation: false,
    );

    Timer(const Duration(seconds: 5), () {
      if (!_isConnected) _startAutoScan();
    });
  }

  Future<void> _connectToDevice(BluetoothDevice device) async {
    try {
      FlutterBluePlus.stopScan();
      setState(() => _isScanning = false);

      await device.connect(autoConnect: false);
      setState(() {
        _connectedDevice = device;
        _isConnected = true;
        _statusMessage = "Discovering services...";
      });

      List<BluetoothService> services = await device.discoverServices();
      for (BluetoothService service in services) {
        if (service.serviceUuid.toString().toLowerCase() == _serviceUUID.toLowerCase()) {
          for (BluetoothCharacteristic characteristic in service.characteristics) {
            if (characteristic.characteristicUuid.toString().toLowerCase() == _characteristicUUID.toLowerCase()) {
              _targetCharacteristic = characteristic;
              
              await characteristic.setNotifyValue(true);
              _characteristicSubscription = characteristic.onValueReceived.listen((value) {
                _processData(String.fromCharCodes(value));
              });
              
              setState(() => _statusMessage = "Connected and ready");
              return;
            }
          }
        }
      }
      
      setState(() => _statusMessage = "Service not found");
    } catch (e) {
      setState(() {
        _isConnected = false;
        _statusMessage = "Connection failed: ${e.toString().split(':').last}";
      });
      _startAutoScan();
    }
  }

  void _processData(String jsonString) {
    try {
      Map<String, dynamic> data = jsonDecode(jsonString);
      final isCalibration = data['mood']?.toString().toLowerCase() == "calibrating...";
      
      final newPoint = HealthDataPoint(
        timestamp: DateTime.now(),
        heartRate: data['hr']?.toDouble() ?? 0,
        temperature: data['temp']?.toDouble() ?? 0,
        gsr: data['gsr'] ?? 0,
        mood: data['mood'] ?? "Unknown",
        isCalibration: isCalibration,
        batteryPercentage: data['battery'] ?? 0,
      );

      setState(() {
        _heartRate = newPoint.heartRate;
        _temperature = newPoint.temperature;
        _gsr = newPoint.gsr;
        _mood = newPoint.mood;
        _progress = data['progress']?.toDouble() ?? 0;
        _fingerDetected = data['finger'] ?? false;
        _batteryPercentage = newPoint.batteryPercentage;
        
        if (!_fingerDetected) {
          _statusMessage = _batteryPercentage < 20 
            ? "Place finger on sensor (Low Battery: $_batteryPercentage%)"
            : "Place finger on sensor";
          _dataCollectionTimer?.cancel();
        } else {
          _statusMessage = isCalibration 
              ? "Calibration in progress..." 
              : _batteryPercentage < 15
                ? "Monitoring... (Critical Battery: $_batteryPercentage%)"
                : "Monitoring health data...";
          _addDataPoint(newPoint);
          
          // Start periodic data collection if not already running
          _dataCollectionTimer ??= Timer.periodic(const Duration(seconds: 30), (_) {
            _addDataPoint(newPoint);
          });
        }
      });
    } catch (e) {
      debugPrint("Data parsing error: $e");
    }
  }

  Future<void> _disconnect() async {
    try {
      _dataCollectionTimer?.cancel();
      _characteristicSubscription?.cancel();
      await _connectedDevice?.disconnect();
    } finally {
      setState(() {
        _isConnected = false;
        _connectedDevice = null;
        _targetCharacteristic = null;
        _statusMessage = "Disconnected";
        _mood = "Waiting for connection...";
        _heartRate = 0;
        _temperature = 0;
        _gsr = 0;
        _progress = 0;
        _batteryPercentage = 0;
      });
      _startAutoScan();
    }
  }

  Future<void> _generateReport() async {
  if (_healthData.isEmpty) {
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text("No data available to generate report")),
    );
    return;
  }

  // Show date selection dialog
  final selectedDate = await showDatePicker(
    context: context,
    initialDate: DateTime.now().subtract(const Duration(days: 1)),
    firstDate: DateTime.now().subtract(const Duration(days: 30)),
    lastDate: DateTime.now(),
    helpText: 'Select date for report',
  );

  if (selectedDate == null) return;

  // Show loading indicator
  showDialog(
    context: context,
    barrierDismissible: false,
    builder: (BuildContext context) {
      return const AlertDialog(
        content: Row(
          children: [
            CircularProgressIndicator(),
            SizedBox(width: 20),
            Text("Generating report..."),
          ],
        ),
      );
    },
  );

  try {
    final filteredData = HealthDataPoint.filterByDate(_healthData, selectedDate);
    
    // Dismiss loading dialog
    Navigator.of(context).pop();
    
    if (filteredData.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text("No data available for ${DateFormat('MMM d, y').format(selectedDate)}"),
          backgroundColor: Colors.orange,
        ),
      );
      return;
    }

    // Generate PDF with filtered data
    await _generatePDFReport(filteredData, selectedDate);
  } catch (e) {
    // Dismiss loading dialog if still showing
    if (Navigator.canPop(context)) {
      Navigator.of(context).pop();
    }
    
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text("Error generating report: $e"),
        backgroundColor: Colors.red,
      ),
    );
  }
}
  Future<void> _generatePDFReport(List<HealthDataPoint> dayData, DateTime reportDate) async {
  final pdf = pw.Document();
  final font = await PdfGoogleFonts.openSansRegular();
  final fontBold = await PdfGoogleFonts.openSansBold();

  pdf.addPage(
    pw.MultiPage(
      pageFormat: PdfPageFormat.a4,
      theme: pw.ThemeData.withFont(base: font, bold: fontBold),
      build: (pw.Context context) => [
        _buildReportHeader(reportDate),
        _buildDayCharts(dayData),
        _buildDataLog(dayData),
      ],
    ),
  );

  await Printing.sharePdf(
    bytes: await pdf.save(),
    filename: 'health-report-${DateFormat('yyyyMMdd').format(reportDate)}.pdf',
  );
}

pw.Widget _buildReportHeader(DateTime reportDate) {
  return pw.Column(
    crossAxisAlignment: pw.CrossAxisAlignment.start,
    children: [
      pw.Text('DAILY HEALTH REPORT',
          style: pw.TextStyle(fontSize: 18, fontWeight: pw.FontWeight.bold, color: PdfColors.blue800)),
      pw.SizedBox(height: 5),
      pw.Text('Date: ${DateFormat('MMMM d, y').format(reportDate)}',
          style: pw.TextStyle(fontSize: 14, fontWeight: pw.FontWeight.bold)),
      pw.Text('Generated: ${DateFormat('MMM d, y - HH:mm').format(DateTime.now())}',
          style: const pw.TextStyle(fontSize: 10)),
      pw.Divider(),
      pw.SizedBox(height: 20),
    ],
  );
}

pw.Widget _buildDayCharts(List<HealthDataPoint> data) {
  // Sort data by time
  data.sort((a, b) => a.timestamp.compareTo(b.timestamp));
  
  return pw.Column(
    crossAxisAlignment: pw.CrossAxisAlignment.start,
    children: [
      pw.Header(level: 1, child: pw.Text('Trends Throughout the Day')),
      pw.SizedBox(height: 10),
      
      // Create time-based data points for visualization
      pw.Table(
        border: pw.TableBorder.all(color: PdfColors.grey300),
        children: [
          pw.TableRow(
            decoration: pw.BoxDecoration(color: PdfColors.blue800),
            children: [
              pw.Padding(padding: const pw.EdgeInsets.all(8), 
                child: pw.Text('Time', style: pw.TextStyle(color: PdfColors.white, fontWeight: pw.FontWeight.bold))),
              pw.Padding(padding: const pw.EdgeInsets.all(8), 
                child: pw.Text('HR (BPM)', style: pw.TextStyle(color: PdfColors.white, fontWeight: pw.FontWeight.bold))),
              pw.Padding(padding: const pw.EdgeInsets.all(8), 
                child: pw.Text('Temp (°C)', style: pw.TextStyle(color: PdfColors.white, fontWeight: pw.FontWeight.bold))),
              pw.Padding(padding: const pw.EdgeInsets.all(8), 
                child: pw.Text('GSR', style: pw.TextStyle(color: PdfColors.white, fontWeight: pw.FontWeight.bold))),
              pw.Padding(padding: const pw.EdgeInsets.all(8), 
                child: pw.Text('Mood', style: pw.TextStyle(color: PdfColors.white, fontWeight: pw.FontWeight.bold))),
            ],
          ),
          // Show data points every hour or significant changes
          ...data.asMap().entries.where((entry) {
            final index = entry.key;
            if (index == 0) return true; // First point
            if (index == data.length - 1) return true; // Last point
            
            // Show points with significant changes or every 10th point
            final current = entry.value;
            final previous = data[index - 1];
            return index % 10 == 0 || 
                   (current.heartRate - previous.heartRate).abs() > 10 ||
                   (current.temperature - previous.temperature).abs() > 0.5 ||
                   current.mood != previous.mood;
          }).map((entry) {
            final point = entry.value;
            return pw.TableRow(
              children: [
                pw.Padding(padding: const pw.EdgeInsets.all(6), 
                  child: pw.Text(DateFormat('HH:mm').format(point.timestamp))),
                pw.Padding(padding: const pw.EdgeInsets.all(6), 
                  child: pw.Text(point.heartRate.toStringAsFixed(0))),
                pw.Padding(padding: const pw.EdgeInsets.all(6), 
                  child: pw.Text(point.temperature.toStringAsFixed(1))),
                pw.Padding(padding: const pw.EdgeInsets.all(6), 
                  child: pw.Text(point.gsr.toString())),
                pw.Padding(padding: const pw.EdgeInsets.all(6), 
                  child: pw.Text(point.mood)),
              ],
            );
          }).toList(),
        ],
      ),
      pw.SizedBox(height: 20),
      
      // Summary statistics
      _buildDaySummary(data),
      pw.SizedBox(height: 20),
    ],
  );
}

pw.Widget _buildDaySummary(List<HealthDataPoint> data) {
  final avgHr = _calculateAverage(data.map((e) => e.heartRate).toList());
  final avgTemp = _calculateAverage(data.map((e) => e.temperature).toList());
  final avgGsr = _calculateAverage(data.map((e) => e.gsr.toDouble()).toList());
  final dominantMood = _getDominantMood(data);

  return pw.Container(
    decoration: pw.BoxDecoration(
      color: PdfColors.blue50,
      borderRadius: pw.BorderRadius.circular(8),
    ),
    padding: const pw.EdgeInsets.all(16),
    child: pw.Column(
      crossAxisAlignment: pw.CrossAxisAlignment.start,
      children: [
        pw.Text('Daily Summary', style: pw.TextStyle(fontSize: 16, fontWeight: pw.FontWeight.bold)),
        pw.SizedBox(height: 10),
        pw.Row(
          mainAxisAlignment: pw.MainAxisAlignment.spaceAround,
          children: [
            _buildSummaryStat('Avg Heart Rate', '${avgHr.toStringAsFixed(0)} BPM'),
            _buildSummaryStat('Avg Temperature', '${avgTemp.toStringAsFixed(1)}°C'),
            _buildSummaryStat('Avg GSR', avgGsr.toStringAsFixed(0)),
            _buildSummaryStat('Dominant Mood', dominantMood),
          ],
        ),
      ],
    ),
  );
}

pw.Widget _buildDataLog(List<HealthDataPoint> data) {
  return pw.Column(
    crossAxisAlignment: pw.CrossAxisAlignment.start,
    children: [
      pw.Header(level: 1, child: pw.Text('Complete Data Log')),
      pw.Text('All ${data.length} data points collected', style: const pw.TextStyle(fontSize: 12)),
      pw.SizedBox(height: 10),
      
      pw.Table(
        border: pw.TableBorder.all(color: PdfColors.grey300),
        columnWidths: {
          0: const pw.FlexColumnWidth(2),
          1: const pw.FlexColumnWidth(1.5),
          2: const pw.FlexColumnWidth(1.5),
          3: const pw.FlexColumnWidth(1),
          4: const pw.FlexColumnWidth(2),
        },
        children: [
          pw.TableRow(
            decoration: pw.BoxDecoration(color: PdfColors.grey800),
            children: [
              pw.Padding(padding: const pw.EdgeInsets.all(4), 
                child: pw.Text('Timestamp', style: pw.TextStyle(color: PdfColors.white, fontSize: 10, fontWeight: pw.FontWeight.bold))),
              pw.Padding(padding: const pw.EdgeInsets.all(4), 
                child: pw.Text('HR', style: pw.TextStyle(color: PdfColors.white, fontSize: 10, fontWeight: pw.FontWeight.bold))),
              pw.Padding(padding: const pw.EdgeInsets.all(4), 
                child: pw.Text('Temp', style: pw.TextStyle(color: PdfColors.white, fontSize: 10, fontWeight: pw.FontWeight.bold))),
              pw.Padding(padding: const pw.EdgeInsets.all(4), 
                child: pw.Text('GSR', style: pw.TextStyle(color: PdfColors.white, fontSize: 10, fontWeight: pw.FontWeight.bold))),
              pw.Padding(padding: const pw.EdgeInsets.all(4), 
                child: pw.Text('Mood', style: pw.TextStyle(color: PdfColors.white, fontSize: 10, fontWeight: pw.FontWeight.bold))),
            ],
          ),
          ...data.map((point) => pw.TableRow(
            children: [
              pw.Padding(padding: const pw.EdgeInsets.all(4), 
                child: pw.Text(DateFormat('HH:mm:ss').format(point.timestamp), style: const pw.TextStyle(fontSize: 9))),
              pw.Padding(padding: const pw.EdgeInsets.all(4), 
                child: pw.Text(point.heartRate.toStringAsFixed(0), style: const pw.TextStyle(fontSize: 9))),
              pw.Padding(padding: const pw.EdgeInsets.all(4), 
                child: pw.Text(point.temperature.toStringAsFixed(1), style: const pw.TextStyle(fontSize: 9))),
              pw.Padding(padding: const pw.EdgeInsets.all(4), 
                child: pw.Text(point.gsr.toString(), style: const pw.TextStyle(fontSize: 9))),
              pw.Padding(padding: const pw.EdgeInsets.all(4), 
                child: pw.Text(point.mood, style: const pw.TextStyle(fontSize: 9))),
            ],
          )).toList(),
        ],
      ),
    ],
  );
}
 // Data analysis helper methods
  double _calculateAverage(List<double> values) {
    if (values.isEmpty) return 0;
    return values.reduce((a, b) => a + b) / values.length;
  }
  pw.Widget _buildSummaryStat(String label, String value) {
    return pw.Column(
      children: [
        pw.Text(label, style: const pw.TextStyle(fontSize: 9)),
        pw.Text(value,
            style: pw.TextStyle(fontSize: 14, fontWeight: pw.FontWeight.bold)),
      ],
    );
  }
  String _getStabilityIndicator(List<double> values) {
    if (values.length < 2) return 'Insufficient Data';
    final avg = _calculateAverage(values);
    final variance = values.map((v) => (v - avg).abs()).reduce((a, b) => a + b) / values.length;
    if (variance < avg * 0.05) return 'Excellent';
    if (variance < avg * 0.1) return 'Good';
    if (variance < avg * 0.2) return 'Fair';
    return 'Variable';
  }

  String _getCalibrationQuality(List<HealthDataPoint> data) {
    if (data.isEmpty) return 'No Calibration Data';
    final hrStability = _getStabilityIndicator(data.map((e) => e.heartRate).toList());
    final tempStability = _getStabilityIndicator(data.map((e) => e.temperature).toList());
    
    if (hrStability == 'Excellent' && tempStability == 'Excellent') {
      return 'Excellent - Optimal calibration';
    } else if (hrStability == 'Variable' || tempStability == 'Variable') {
      return 'Poor - Recommend recalibration';
    }
    return 'Adequate - Suitable for monitoring';
  }

  PdfColor _getCalibrationQualityColor(List<HealthDataPoint> data) {
    final quality = _getCalibrationQuality(data);
    if (quality.contains('Excellent')) return PdfColors.green;
    if (quality.contains('Poor')) return PdfColors.red;
    return PdfColors.orange;
  }

  String _getDominantMood(List<HealthDataPoint> data) {
    if (data.isEmpty) return 'No Data';
    final moodCounts = <String, int>{};
    for (var point in data) {
      moodCounts[point.mood] = (moodCounts[point.mood] ?? 0) + 1;
    }
    return moodCounts.entries.reduce((a, b) => a.value > b.value ? a : b).key;
  }

  List<List<String>> _getMoodStatistics(List<HealthDataPoint> data) {
    if (data.isEmpty) return [];
    final moodGroups = groupBy(data, (HealthDataPoint point) => point.mood);
    return moodGroups.entries.map((entry) {
      final points = entry.value;
      return [
        entry.key,
        '${points.length} (${((points.length / data.length) * 100).toStringAsFixed(1)}%)',
        '${_calculateAverage(points.map((e) => e.heartRate).toList()).toStringAsFixed(1)} BPM\n'
            '${_calculateAverage(points.map((e) => e.temperature).toList()).toStringAsFixed(1)}°C',
      ];
    }).toList();
  }

  PdfColor _getDayRowColor(String mood) {
    switch (mood.toLowerCase()) {
      case 'happy':
        return PdfColors.green50;
      case 'stressed':
        return PdfColors.red50;
      case 'sad':
        return PdfColors.blue50;
      case 'calm':
        return PdfColors.purple50;
      default:
        return PdfColors.white;
    }
  }

  String _getHeartRateNotes(List<double> values) {
    final avg = _calculateAverage(values);
    if (avg > 90) return 'Elevated baseline';
    if (avg < 60) return 'Low baseline';
    return 'Normal range';
  }

  String _getTemperatureNotes(List<double> values) {
    final avg = _calculateAverage(values);
    if (avg > 37.2) return 'Elevated';
    if (avg < 36.1) return 'Low';
    return 'Normal';
  }

  String _getGsrNotes(List<double> values) {
    final avg = _calculateAverage(values);
    if (avg > 600) return 'High stress';
    if (avg < 200) return 'Low response';
    return 'Normal range';
  }

  String _getMoodRecommendation(String mood) {
    switch (mood.toLowerCase()) {
      case 'stressed':
        return 'Recommend stress management techniques and follow-up evaluation';
      case 'sad':
        return 'Consider mood assessment and counseling referral';
      case 'happy':
        return 'Positive mood patterns observed';
      case 'calm':
        return 'Relaxed state predominant';
      default:
        return 'Monitor for mood changes';
    }
  }

  @override
  void dispose() {
    _dataCollectionTimer?.cancel();
    _scanSubscription?.cancel();
    _characteristicSubscription?.cancel();
    //_batteryUpdateTimer?.cancel();
    _connectedDevice?.disconnect();
    super.dispose();
  }

  String _getMoodEmoji(String mood) {
    switch (mood.toLowerCase()) {
      case "happy": return "😊";
      case "sad": return "😢";
      case "stressed": return "😰";
      case "calm": return "😌";
      case "calibrating...": return "🔄";
      case "monitoring...": return "🔍";
      default: return "🤔";
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: Colors.grey[50],
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            children: [
              // Header with connection status
              Row(
                mainAxisAlignment: MainAxisAlignment.spaceBetween,
                children: [
                  const Text(
                    'Health Monitor',
                    style: TextStyle(
                      fontSize: 24,
                      fontWeight: FontWeight.bold,
                      color: Colors.black87,
                    ),
                  ),
                  Row(
  children: [
    Icon(
      _isConnected ? Icons.bluetooth_connected : Icons.bluetooth,
      color: _isConnected ? Colors.blue : Colors.grey,
      size: 24,
    ),
    const SizedBox(width: 16),
    IconButton(
      icon: const Icon(Icons.insert_chart, size: 24),
      onPressed: _generateReport,
      color: Colors.black87,
    ),
    IconButton(
      icon: const Icon(Icons.delete_forever, size: 24),
      onPressed: _clearAllData,
      color: Colors.red[400],
    ),
  ],
),
                ],
              ),
              const SizedBox(height: 16),
              
              // Mood display
              Container(
                padding: const EdgeInsets.all(16),
                decoration: BoxDecoration(
                  color: Colors.white,
                  borderRadius: BorderRadius.circular(16),
                  boxShadow: [
                    BoxShadow(
                      color: Colors.grey.withOpacity(0.1),
                      blurRadius: 10,
                      offset: const Offset(0, 4),
                    ),
                  ],
                ),
                child: Column(
                  children: [
                    Text(
                      _getMoodEmoji(_mood),
                      style: const TextStyle(fontSize: 48),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      _mood,
                      style: const TextStyle(
                        fontSize: 20,
                        fontWeight: FontWeight.w500,
                        color: Colors.black87,
                      ),
                    ),
                    const SizedBox(height: 12),
                    if (_fingerDetected)
                      LinearProgressIndicator(
                        value: _progress / 100,
                        backgroundColor: Colors.grey[200],
                        valueColor: const AlwaysStoppedAnimation<Color>(Colors.blue),
                      ),
                  ],
                ),
              ),
              const SizedBox(height: 16),
              
              // Sensor grid
            Expanded(
  child: SingleChildScrollView( // Add this to enable scrolling
    child: Column(
      children: [
        GridView.count(
          shrinkWrap: true, // Important for scrolling
          physics: const NeverScrollableScrollPhysics(), // Disable nested scrolling
          crossAxisCount: 2,
          childAspectRatio: 0.9,
          crossAxisSpacing: 12,
          mainAxisSpacing: 12,
          padding: const EdgeInsets.only(bottom: 12), // Add some bottom padding
          children: [
            _buildSensorCard(
              title: "HEART RATE",
              value: "${_heartRate.toStringAsFixed(0)}",
              unit: "BPM",
              icon: Icons.favorite,
              color: Colors.red[400]!,
            ),
            _buildSensorCard(
              title: "TEMPERATURE",
              value: "${_temperature.toStringAsFixed(1)}",
              unit: "°C",
              icon: Icons.thermostat,
              color: Colors.orange[400]!,
            ),
            _buildSensorCard(
              title: "GSR",
              value: _gsr.toString(),
              unit: "",
              icon: Icons.electric_bolt,
              color: Colors.purple[400]!,
            ),
            _buildSensorCard(
              title: "BATTERY",
              value: "$_batteryPercentage",
              unit: "%",
              icon: Icons.battery_full,
              color: _getBatteryColor(_batteryPercentage),
            ),
          ],
        ),
        // Status box (only one instance)
        Container(
          width: double.infinity,
          padding: const EdgeInsets.all(16),
          decoration: BoxDecoration(
            color: Colors.white,
            borderRadius: BorderRadius.circular(16),
            boxShadow: [
              BoxShadow(
                color: Colors.grey.withOpacity(0.1),
                blurRadius: 10,
                offset: const Offset(0, 4),
              ),
            ],
          ),
          child: Row(
            children: [
              Icon(
                Icons.info_outline,
                color: Colors.blue[400],
                size: 32,
              ),
              const SizedBox(width: 16),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      "DEVICE STATUS",
                      style: TextStyle(
                        fontSize: 16,
                        fontWeight: FontWeight.w600,
                        color: Colors.grey[700],
                      ),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      _fingerDetected ? "FINGER DETECTED" : "NO FINGER DETECTED",
                      style: TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.bold,
                        color: _fingerDetected ? Colors.green[600] : Colors.red[600],
                      ),
                    ),
                    const SizedBox(height: 4),
                    Text(
                      _statusMessage,
                      style: TextStyle(
                        fontSize: 14,
                        color: Colors.grey[600],
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ],
    ),
  ),
),
        // Data points counter (only one instance)
              
              // Data points counter
              Container(
                padding: const EdgeInsets.symmetric(vertical: 8, horizontal: 16),
                decoration: BoxDecoration(
                  color: Colors.white,
                  borderRadius: BorderRadius.circular(12),
                  boxShadow: [
                    BoxShadow(
                      color: Colors.grey.withOpacity(0.1),
                      blurRadius: 6,
                      offset: const Offset(0, 2),
                    ),
                  ],
                ),
                child: Row(
                  mainAxisAlignment: MainAxisAlignment.spaceBetween,
                  children: [
                    Text(
                      'Data points collected',
                      style: TextStyle(
                        color: Colors.grey[600],
                        fontSize: 14,
                      ),
                    ),
                    Text(
                      _healthData.length.toString(),
                      style: const TextStyle(
                        fontWeight: FontWeight.bold,
                        fontSize: 16,
                        color: Colors.black87,
                      ),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildSensorCard({
    required String title,
    required String value,
    required String unit,
    required IconData icon,
    required Color color,
  }) {
    return Container(
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: Colors.white,
        borderRadius: BorderRadius.circular(16),
        boxShadow: [
          BoxShadow(
            color: Colors.grey.withOpacity(0.1),
            blurRadius: 10,
            offset: const Offset(0, 4),
          ),
        ],
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        mainAxisAlignment: MainAxisAlignment.spaceBetween,
        children: [
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              Text(
                title,
                style: TextStyle(
                  fontSize: 14,
                  fontWeight: FontWeight.w500,
                  color: Colors.grey[600],
                ),
              ),
              Icon(icon, color: color, size: 20),
            ],
          ),
          Row(
            crossAxisAlignment: CrossAxisAlignment.end,
            children: [
              Text(
                value,
                style: const TextStyle(
                  fontSize: 24,
                  fontWeight: FontWeight.bold,
                  color: Colors.black87,
                ),
              ),
              const SizedBox(width: 4),
              Padding(
                padding: const EdgeInsets.only(bottom: 4),
                child: Text(
                  unit,
                  style: TextStyle(
                    fontSize: 14,
                    color: Colors.grey[600],
                  ),
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }
}

class HealthMonitorPage extends StatefulWidget {
  const HealthMonitorPage({Key? key}) : super(key: key);

  @override
  State<HealthMonitorPage> createState() => _HealthMonitorPageState();
}