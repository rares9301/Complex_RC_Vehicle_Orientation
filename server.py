# -*- coding: utf-8 -*-
from flask import Flask, request, jsonify, send_from_directory
from werkzeug.utils import secure_filename
import numpy as np
import os
import cv2  # Import the OpenCV library for image processing
import cv2.aruco as aruco  # Import the ArUco marker detection functionality from OpenCV

# Initialize a Flask application
app = Flask(__name__)

# Define the folder where uploaded files will be stored
UPLOAD_FOLDER = 'uploads'
# Specify the allowed file extensions for uploaded images
ALLOWED_EXTENSIONS = {'png', 'jpg', 'jpeg', 'gif'}
# Configure the application to use the defined upload folder
app.config['UPLOAD_FOLDER'] = UPLOAD_FOLDER
# Create the upload folder if it doesn't already exist
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

aruco_messages = {
    'last_message': ""
}

def detect_aruco_markers(image_path):
    """
    Detect ArUco markers in an image and print details about detected markers.

    Args:
    image_path (str): The file path of the image to analyze.
    """
    # Load the image from the specified path
    frame = cv2.imread(image_path)
    # Get a pre-defined dictionary of ArUco markers
    dictionary = aruco.getPredefinedDictionary(aruco.DICT_6X6_250)
    # Create detector parameters
    parameters = aruco.DetectorParameters_create() if hasattr(aruco, 'DetectorParameters_create') else aruco.DetectorParameters()
    
    # Detect ArUco markers in the image
    markerCorners, markerIds, rejectedCandidates = aruco.detectMarkers(frame, dictionary, parameters=parameters)
    
    aruco_messages['last_message'] = []

    if markerIds is not None and len(markerIds) > 1:
        centers = [np.mean(corner, axis=1) for corner in markerCorners]
        num_markers = len(centers)
        marker_ids = [str(id[0]) for id in markerIds]
        distances = []

        for i in range(num_markers):
            for j in range(i + 1, num_markers):
                distance = np.linalg.norm(centers[i] - centers[j])
                distances.append(f"Distanta intre markerul {markerIds[i][0]} si {markerIds[j][0]} este {distance:.2f} pixeli.")

        marker_message = f"In imaginea {image_path} au fost detectate markerle ArUco cu id-ul " + " | ".join(marker_ids) + ";"
        aruco_messages['last_message'].extend(distances)
        full_message = marker_message + " " + " ".join(distances)
        print(full_message)
    elif markerIds is not None:
        one_aruco_message = f"- V - In imaginea {image_path} a fost detectat un marker ArUco cu id-ul {markerIds[0][0]}."
        aruco_messages['last_message'].append(one_aruco_message)
        print(one_aruco_message)
    else:
        no_aruco_message = f"- X - Niciun marker ArUco nu a fost detectat in imaginea {image_path}."
        aruco_messages['last_message'].append(no_aruco_message)
        print(no_aruco_message)

def allowed_file(filename):
    """
    Check if the file has an allowed extension.

    Args:
    filename (str): The name of the file to check.

    Returns:
    bool: True if the file's extension is allowed, False otherwise.
    """
    return '.' in filename and filename.rsplit('.', 1)[1].lower() in ALLOWED_EXTENSIONS

@app.route('/')
def index():
    """Define the root route of the web application."""
    return "Hello, World! The Flask server is running."

@app.route('/test', methods=['GET'])
def test_connection():
    """Define a route to test the connection to the server."""
    print("Received a test connection request.")
    response = jsonify({'message': 'Success! Flutter app connected to Flask server.'}), 200
    if response[1] == 200:
        print("Test connection successful.")
    return response

@app.route('/get_aruco_status', methods=['GET'])
def get_aruco_status():
    if aruco_messages['last_message']:
        return jsonify({'distances': aruco_messages['last_message']})
    else:
        return jsonify({'message': 'Nu exista informatii recente despre markere.'})

@app.route('/upload', methods=['POST'])
def upload_file():
    """Define a route to upload an image file."""
    # Check if the request contains an 'image' part
    if 'image' not in request.files:
        return jsonify({'message': 'No image part'}), 400
    file = request.files['image']
    # Check if a file was actually selected/uploaded
    if file.filename == '':
        return jsonify({'message': 'No selected image'}), 400
    # Save the file if it has an allowed extension and return a success response
    if file and allowed_file(file.filename):
        filename = secure_filename(file.filename)
        filepath = os.path.join(app.config['UPLOAD_FOLDER'], filename)
        file.save(filepath)
        response = jsonify({'message': 'Image uploaded successfully', 'filename': filename})
        if response.status_code == 200:
            print("----------------------------------------------------------")
            print(f"Image uploaded successfully: {filename}")
            # After saving the file, attempt to detect ArUco markers in it
            detect_aruco_markers(filepath)
        return response, 200
    else:
        return jsonify({'message': 'Unsupported file type'}), 400

@app.route('/uploads/<filename>')
def uploaded_file(filename):
    """Serve files from the upload directory."""
    return send_from_directory(app.config['UPLOAD_FOLDER'], filename)

if __name__ == '__main__':
    # Run the application with debug mode enabled and accessible from any host
    app.run(debug=True, host='0.0.0.0', port=5000)
