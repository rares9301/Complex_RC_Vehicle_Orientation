// Importing necessary Dart and Flutter packages for camera access, HTTP requests, and permissions handling.
import 'dart:async';
import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:camera/camera.dart';
import 'package:http/http.dart' as http;
import 'package:permission_handler/permission_handler.dart';

// Global variable for storing available camera descriptions.
List<CameraDescription>? cameras;

// The main asynchronous entry point of the application.
Future<void> main() async {
  // Ensuring that widget binding is initialized before setting up cameras.
  WidgetsFlutterBinding.ensureInitialized();
  // Fetching available cameras and initializing the camera list.
  cameras = await availableCameras();
  // Running the application with MyApp as the root widget.
  runApp(const MyApp());
}

// The main application widget, setting up the theme and home screen.
class MyApp extends StatelessWidget {
  const MyApp({Key? key}) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Camera to Server', // Application title.
      theme: ThemeData( // Application theme.
        primarySwatch: Colors.blue,
        visualDensity: VisualDensity.adaptivePlatformDensity,
      ),
      home: const CameraScreen(), // The home screen of the app, showing the camera view.
    );
  }
}

// The camera screen widget, responsible for displaying the camera view and uploading images.
class CameraScreen extends StatefulWidget {
  const CameraScreen({Key? key}) : super(key: key);

  @override
  _CameraScreenState createState() => _CameraScreenState();
}

class _CameraScreenState extends State<CameraScreen> {
  CameraController? controller; // Controller for managing camera state.
  Timer? timer; // Timer for periodically capturing images.
  bool isUploading = false; // Flag to track upload status.

  @override
  void initState() {
    super.initState();
    _requestPermissions(); // Requesting necessary permissions on init.
  }

  @override
  void dispose() {
    // Disposing of the controller and timer when the widget is removed from the widget tree.
    controller?.dispose();
    timer?.cancel();
    super.dispose();
  }

  // Asynchronously requests camera, microphone, and storage permissions.
  Future<void> _requestPermissions() async {
    await [Permission.camera, Permission.microphone, Permission.storage].request();
    _initCamera(); // Initializes the camera after permissions are granted.
  }

  // Initializes the camera controller with the first available camera and sets resolution.
  void _initCamera() async {
    controller = CameraController(cameras![0], ResolutionPreset.medium);
    await controller!.initialize();
    await controller!.setFlashMode(FlashMode.off); // Disabling flash.
    if (!mounted) {
      return;
    }
    setState(() {}); // Refreshing the UI after initialization.
  }

  // Toggles the uploading flag and starts/stops the image capture loop.
  void _toggleUploading() {
    setState(() {
      isUploading = !isUploading;
      if (isUploading) {
        _startImageCaptureLoop(); // Starts capturing images if uploading is enabled.
      } else {
        timer?.cancel(); // Stops the timer if uploading is disabled.
      }
    });
  }

  // Starts a periodic timer that captures an image every 1 second.
  void _startImageCaptureLoop() {
    timer = Timer.periodic(Duration(seconds: 1), (Timer t) => _takePicture());
  }

  // Captures an image and, if uploading is enabled, sends it to the server.
  Future<void> _takePicture() async {
    if (!controller!.value.isInitialized || !isUploading) {
      return;
    }
    try {
      final image = await controller!.takePicture();
      if (isUploading) {
        _sendImageToServer(image.path); // Sends the captured image to the server.
      }
    } catch (e) {
      print("Failed to take picture: $e");
    }
  }

  // Asynchronously sends the captured image to the server.
  Future<void> _sendImageToServer(String imagePath) async {
    if (!isUploading) return; // If uploading is stopped, the function returns early.
  
    var uri = Uri.parse('http://192.168.100.47:5000/upload'); // Server URL.
    var request = http.MultipartRequest('POST', uri)
        ..files.add(await http.MultipartFile.fromPath('image', imagePath)); // Creating a multipart request with the image.

    try {
        var streamedResponse = await request.send();
        var response = await http.Response.fromStream(streamedResponse);
        if (response.statusCode == 200) {
            print(jsonDecode(response.body)['message']); // Prints server response on success.
        } else {
            // If the server responds with an error code, log the error along with the status code.
            print("Error sending the image to the server. Status code: ${response.statusCode}");
        }
    } catch (e) {
        // Catching and logging any exceptions that occur during the upload process.
        print("Exception caught: $e");
    }
  }

  @override
  Widget build(BuildContext context) {
    // Building the UI of the camera screen.
    return Scaffold(
      appBar: AppBar(
        title: const Text('Camera to Server'), // AppBar title.
      ),
      body: controller == null || !controller!.value.isInitialized
          ? const Center(child: CircularProgressIndicator()) // Show loading spinner until the camera is initialized.
          : CameraPreview(controller!), // Displaying the camera preview if the controller is initialized.
      floatingActionButton: FloatingActionButton(
        onPressed: _toggleUploading,
        child: Icon(isUploading ? Icons.pause : Icons.cloud_upload), // Changing icon based on uploading status.
      ),
      floatingActionButtonLocation: FloatingActionButtonLocation.centerFloat, // Positioning the FAB at the bottom center.
    );
  }
}

