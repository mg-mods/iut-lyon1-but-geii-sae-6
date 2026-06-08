import face_recognition
import cv2
import numpy as np
import time
import pickle
from gpiozero import LED
from time import sleep

# Choose mode (all can be false, but if several are true then the first one will be used); default: FDM
fr_mode_fdm = True # Full Display Mode: displays rectangles around each subject's head
fr_mode_mdb = False # Minimal Display Mode: displays the names of the detected subjects in the top right corner

# Choose output pins
com_high = 17 # Pin 11 (GPIO_17) used as HIGH communication pin by default
com_low = 27 # Pin 13 (GPIO_27) used as LOW communication pin by default
com_data = 22 # Pin 15 (GPIO_22 used as DATA communication pin by default


# Load pre-trained face encodings
print("[INFO] loading encodings...")
with open("encodings.pickle", "rb") as f:
    data = pickle.loads(f.read())
known_face_encodings = data["encodings"]
known_face_names = data["names"]

# Initialize the USB webcam (0 is usually the default camera index)
cap = cv2.VideoCapture(0)

# Set resolution (optional)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

# Initialize our variables
cv_scaler = 4 # this has to be a whole number

# Initialize GPIO
pin_high = LED(com_high)
pin_low = LED(com_low)
pin_high.on()
pin_low.off()

face_locations = []
face_encodings = []
face_names = []
frame_count = 0
start_time = time.time()

def process_frame(frame):
    global face_locations, face_encodings, face_names
    
    # Resize the frame using cv_scaler to increase performance (less pixels processed, less time spent)
    resized_frame = cv2.resize(frame, (0, 0), fx=(1/cv_scaler), fy=(1/cv_scaler))
    
    # Convert the image from BGR to RGB colour space, the facial recognition library uses RGB, OpenCV uses BGR
    rgb_resized_frame = cv2.cvtColor(resized_frame, cv2.COLOR_BGR2RGB)
    
    # Find all the faces and face encodings in the current frame of video
    face_locations = face_recognition.face_locations(rgb_resized_frame)
    face_encodings = face_recognition.face_encodings(rgb_resized_frame, face_locations, model='large')
    
    face_names = []
    for face_encoding in face_encodings:
        # See if the face is a match for the known face(s)
        matches = face_recognition.compare_faces(known_face_encodings, face_encoding)
        name = "Unknown"
        
        # Use the known face with the smallest distance to the new face
        face_distances = face_recognition.face_distance(known_face_encodings, face_encoding)
        best_match_index = np.argmin(face_distances)
        if matches[best_match_index]:
            name = known_face_names[best_match_index]
        face_names.append(name)
    
    return frame

def draw_results(frame):
    # Display the results
    for (top, right, bottom, left), name in zip(face_locations, face_names):
        # Scale back up face locations since the frame we detected in was scaled
        top *= cv_scaler
        right *= cv_scaler
        bottom *= cv_scaler
        left *= cv_scaler
        rectColorR = 30
        rectColorG = 255
        rectColorB = 0
        txtColorRGB = 0

        if name == "Unknown":
            rectColorR = 255
            rectColorG = 0
            rectColorB = 10
            txtColorRGB = 255

        # Draw a box around the face
        cv2.rectangle(frame, (left, top), (right, bottom), (rectColorB, rectColorG, rectColorR), 2)
        
        # Draw a label with a name below the face
        cv2.rectangle(frame, (left -3, top - 35), (right+3, top), (rectColorB, rectColorG, rectColorR), cv2.FILLED)
        font = cv2.FONT_HERSHEY_DUPLEX
        cv2.putText(frame, name, (left + 6, top - 6), font, 1.0, (txtColorRGB, txtColorRGB, txtColorRGB), 2)
        
    return frame

def results_minimal():
       
    known = (", ".join(sorted(name for name in face_names if name != "Unknown")))
    if known:
        result = "Detected faces: " + known
    else:
        result = "No known face in frame"
    return result

def setComState(state):
    if state == True:
        pin_high.off()
        pin_low.on()
        sleep(0.01)
    else: 
        pin_high.on()
        pin_low.off()
        sleep(0.01)
    return state

def checkIsFaceDetected():
    return any(name != "Unknown" for name in face_names)

def checkIsFaceDetectedTxt():
    return "On" if any(name != "Unknown" for name in face_names) else "Off"
while True:
    # Capture a frame from camera
    #frame = picam2.capture_array()
    ret,frame = cap.read()
    
    # Process the frame with the function
    processed_frame = process_frame(frame)
    
    # Get the text and boxes to be drawn based on the processed frame
    display_frame = processed_frame
    if fr_mode_fdm == True:
        display_frame = draw_results(processed_frame)
        cv2.putText(frame, "FDM", (0, 15), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
    if fr_mode_mdb == True:
        cv2.putText(frame, results_minimal(), (120, 15), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
        cv2.putText(frame, "MDM", (0, 15), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)

    # Display everything over the video feed.
    cv2.putText(frame, ("COM: " + checkIsFaceDetectedTxt()), (40, 15), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1)
    cv2.imshow('Face Rec Running', display_frame)
    
    # Update communication pins
    setComState(checkIsFaceDetected())
    
    # Change mode if 'm' is pressed
    if cv2.waitKey(1) == ord("m"):
        fr_mode_fdm = not fr_mode_fdm
        fr_mode_mdb = not fr_mode_mdb
    
    # Break the loop and stop the script if 'q' is pressed
    if cv2.waitKey(1) == ord("q"):
        break

# By breaking the loop we run this code here which closes everything
cap.release()
cv2.destroyAllWindows()
#picam2.stop()
