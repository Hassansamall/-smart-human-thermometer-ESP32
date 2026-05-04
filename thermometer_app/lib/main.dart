import 'package:flutter/material.dart';
import 'package:supabase_flutter/supabase_flutter.dart';
import 'package:intl/intl.dart';

Future<void> main() async {
  WidgetsFlutterBinding.ensureInitialized();

  await Supabase.initialize(
    url: 'https://hoqpukivoaialazemrwg.supabase.co',
    anonKey:
        'eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImhvcXB1a2l2b2FpYWxhemVtcndnIiwicm9sZSI6ImFub24iLCJpYXQiOjE3NzQ0NTkwMzAsImV4cCI6MjA5MDAzNTAzMH0.jgNohwkytZTXY9ZP3dcYh1SYyvDgaEdH4d6vhGkn-_Y',
  );

  runApp(const SmartThermometerApp());
}

class SmartThermometerApp extends StatelessWidget {
  const SmartThermometerApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Smart Thermometer',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(
          seedColor: Colors.blue,
          brightness: Brightness.dark,
        ),
        fontFamily: 'Inter',
        useMaterial3: true,
      ),
      home: const DashboardPage(),
    );
  }
}

class DashboardPage extends StatefulWidget {
  const DashboardPage({super.key});

  @override
  _DashboardPageState createState() => _DashboardPageState();
}

class _DashboardPageState extends State<DashboardPage>
    with SingleTickerProviderStateMixin {
  final _supabase = Supabase.instance.client;
  late final Stream<List<Map<String, dynamic>>> _readingsStream;

  @override
  void initState() {
    super.initState();
    // Subscribe to real-time changes
    _readingsStream = _supabase
        .from('temperature_readings')
        .stream(primaryKey: ['id'])
        .order('created_at', ascending: false)
        .limit(20);
  }

  Color _getStatusColor(String status) {
    switch (status) {
      case 'LOW':
        return Colors.blueAccent;
      case 'NORMAL':
        return Colors.greenAccent;
      case 'ELEVATED':
        return Colors.orangeAccent;
      case 'FEVER':
        return Colors.redAccent;
      default:
        return Colors.grey;
    }
  }

  Future<void> _updatePersonName(String id, String currentName) async {
    TextEditingController controller = TextEditingController(
        text: currentName == 'Unknown' ? '' : currentName);

    await showDialog(
      context: context,
      builder: (context) {
        return AlertDialog(
          backgroundColor: const Color(0xFF1E1E2C),
          title: const Text('Assign Name', style: TextStyle(color: Colors.white)),
          content: TextField(
            controller: controller,
            style: const TextStyle(color: Colors.white),
            decoration: const InputDecoration(
              hintText: 'Enter person\'s name',
              hintStyle: TextStyle(color: Colors.white54),
              enabledBorder: UnderlineInputBorder(
                borderSide: BorderSide(color: Colors.white24),
              ),
              focusedBorder: UnderlineInputBorder(
                borderSide: BorderSide(color: Colors.white),
              ),
            ),
            autofocus: true,
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text('Cancel', style: TextStyle(color: Colors.white54)),
            ),
            ElevatedButton(
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.blueAccent,
                foregroundColor: Colors.white,
              ),
              onPressed: () async {
                final newName = controller.text.trim();
                if (newName.isNotEmpty) {
                  await _supabase
                      .from('temperature_readings')
                      .update({'person_name': newName})
                      .eq('id', id);
                }
                if (mounted) Navigator.pop(context);
              },
              child: const Text('Save'),
            ),
          ],
        );
      },
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF0F0F1A),
      appBar: AppBar(
        title: const Text('Smart Thermometer',
            style: TextStyle(fontWeight: FontWeight.bold, letterSpacing: 1.2)),
        backgroundColor: Colors.transparent,
        elevation: 0,
        centerTitle: true,
      ),
      extendBodyBehindAppBar: true,
      body: StreamBuilder<List<Map<String, dynamic>>>(
        stream: _readingsStream,
        builder: (context, snapshot) {
          if (!snapshot.hasData) {
            return const Center(child: CircularProgressIndicator(color: Colors.white));
          }

          final readings = snapshot.data!;
          if (readings.isEmpty) {
            return const Center(
              child: Text(
                'Waiting for sensor data...',
                style: TextStyle(color: Colors.white54, fontSize: 18),
              ),
            );
          }

          final latest = readings.first;
          final double temp = latest['temperature'] is int
              ? (latest['temperature'] as int).toDouble()
              : latest['temperature'];
          final String status = latest['status'] ?? 'UNKNOWN';
          final Color statusColor = _getStatusColor(status);

          return Stack(
            children: [
              // Dynamic Background Glow
              AnimatedContainer(
                duration: const Duration(milliseconds: 800),
                decoration: BoxDecoration(
                  gradient: RadialGradient(
                    center: const Alignment(0, -0.4),
                    radius: 0.8,
                    colors: [
                      statusColor.withOpacity(0.35),
                      const Color(0xFF0F0F1A),
                    ],
                  ),
                ),
              ),

              SafeArea(
                child: Column(
                  children: [
                    // Main Dashboard Display
                    const SizedBox(height: 20),
                    Center(
                      child: AnimatedContainer(
                        duration: const Duration(milliseconds: 500),
                        width: 250,
                        height: 250,
                        decoration: BoxDecoration(
                          shape: BoxShape.circle,
                          color: Colors.black.withOpacity(0.4),
                          border: Border.all(color: statusColor, width: 6),
                          boxShadow: [
                            BoxShadow(
                              color: statusColor.withOpacity(0.4),
                              blurRadius: 30,
                              spreadRadius: 5,
                            )
                          ],
                        ),
                        child: Column(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Text(
                              '${temp.toStringAsFixed(1)}°C',
                              style: const TextStyle(
                                fontSize: 64,
                                fontWeight: FontWeight.w900,
                                color: Colors.white,
                              ),
                            ),
                            const SizedBox(height: 10),
                            Text(
                              status,
                              style: TextStyle(
                                fontSize: 22,
                                fontWeight: FontWeight.bold,
                                color: statusColor,
                                letterSpacing: 2,
                              ),
                            ),
                          ],
                        ),
                      ),
                    ),
                    const SizedBox(height: 10),
                    Text(
                      'Target: ${latest['person_name']}',
                      style: const TextStyle(
                        fontSize: 16,
                        color: Colors.white70,
                        fontWeight: FontWeight.w500,
                      ),
                    ),
                    const SizedBox(height: 40),

                    // History Title
                    const Padding(
                      padding: EdgeInsets.symmetric(horizontal: 24.0),
                      child: Align(
                        alignment: Alignment.centerLeft,
                        child: Text(
                          'Recent Readings',
                          style: TextStyle(
                            fontSize: 20,
                            fontWeight: FontWeight.bold,
                            color: Colors.white,
                          ),
                        ),
                      ),
                    ),
                    const SizedBox(height: 15),

                    // History ListView
                    Expanded(
                      child: ListView.builder(
                        physics: const BouncingScrollPhysics(),
                        padding: const EdgeInsets.symmetric(horizontal: 20),
                        itemCount: readings.length,
                        itemBuilder: (context, index) {
                          final item = readings[index];
                          final itemTemp = item['temperature'] is int
                              ? (item['temperature'] as int).toDouble()
                              : item['temperature'];
                          final itemStatusColor =
                              _getStatusColor(item['status'] ?? '');
                          final date = DateTime.parse(item['created_at']).toLocal();
                          final formattedDate =
                              DateFormat('MMM d, h:mm a').format(date);

                          return Container(
                            margin: const EdgeInsets.only(bottom: 12),
                            decoration: BoxDecoration(
                              color: Colors.white.withOpacity(0.05),
                              borderRadius: BorderRadius.circular(16),
                              border: Border.all(
                                color: Colors.white.withOpacity(0.1),
                              ),
                            ),
                            child: ListTile(
                              contentPadding: const EdgeInsets.symmetric(
                                  horizontal: 20, vertical: 8),
                              leading: CircleAvatar(
                                backgroundColor: itemStatusColor.withOpacity(0.2),
                                child: Text(
                                  '${itemTemp.toStringAsFixed(1)}°',
                                  style: TextStyle(
                                      color: itemStatusColor,
                                      fontWeight: FontWeight.bold,
                                      fontSize: 14),
                                ),
                              ),
                              title: Text(
                                item['person_name'] ?? 'Unknown',
                                style: const TextStyle(
                                    color: Colors.white,
                                    fontWeight: FontWeight.bold),
                              ),
                              subtitle: Text(
                                formattedDate,
                                style: TextStyle(color: Colors.white.withOpacity(0.5)),
                              ),
                              trailing: Row(
                                mainAxisSize: MainAxisSize.min,
                                children: [
                                  IconButton(
                                    icon: const Icon(Icons.edit,
                                        color: Colors.white54, size: 20),
                                    onPressed: () {
                                      _updatePersonName(
                                          item['id'], item['person_name'] ?? '');
                                    },
                                  ),
                                  IconButton(
                                    icon: const Icon(Icons.delete_outline,
                                        color: Colors.redAccent, size: 20),
                                    onPressed: () async {
                                      await _supabase
                                          .from('temperature_readings')
                                          .delete()
                                          .eq('id', item['id']);
                                    },
                                  ),
                                ],
                              ),
                            ),
                          );
                        },
                      ),
                    ),
                  ],
                ),
              ),
            ],
          );
        },
      ),
    );
  }
}
