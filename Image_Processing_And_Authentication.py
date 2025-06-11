import cv2
import numpy as np
import face_recognition
import os
import requests
import gc


faces_folder = "faces"
if not os.path.exists(faces_folder):
    os.makedirs(faces_folder)


known_face_encodings = []
known_face_names = []


def load_known_faces():
    global known_face_encodings, known_face_names
    known_face_encodings = []
    known_face_names = []
    for filename in os.listdir(faces_folder):
        if filename.endswith(".jpg") or filename.endswith(".png"):
            image = face_recognition.load_image_file(os.path.join(faces_folder, filename))
            encoding = face_recognition.face_encodings(image)
            if encoding:
                known_face_encodings.append(encoding[0])
                known_face_names.append(os.path.splitext(filename)[0])


load_known_faces()


image_url = "http://192.168.4.1/cam-med.jpg"


frame_c = 0
skip_frames = 5


while True:
    try:
        response = requests.get(image_url, timeout=5)
        if response.status_code != 200:
            print("Failed to fetch image from URL")
            continue


        image_array = np.frombuffer(response.content, np.uint8)
        image = cv2.imdecode(image_array, cv2.IMREAD_COLOR)


        if image is None:
            print("No frame received")
            continue


        frame_c += 1
        if frame_c % skip_frames != 0:
            continue


        image = cv2.resize(image, (160, 120))
        rgb_image = cv2.cvtColor(image, cv2.COLOR_BGR2RGB)
        face_locations = face_recognition.face_locations(rgb_image, model = "cnn")
        face_encodings = face_recognition.face_encodings(rgb_image, face_locations)
        saved_face_crop = None


        for (top, right, bottom, left), face_encoding in zip(face_locations, face_encodings):
            matches = face_recognition.compare_faces(known_face_encodings, face_encoding)
            name = "Unknown"
            color = (0, 0, 255)
            face_distances = face_recognition.face_distance(known_face_encodings, face_encoding)
            if len(face_distances) > 0:
                best_match_index = np.argmin(face_distances)
                if matches[best_match_index]:
                    name = known_face_names[best_match_index]
                    color = (0, 255, 0)
                    requests.get('http://192.168.4.1/blink')


            cv2.rectangle(image, (left, top), (right, bottom), color, 2)
            cv2.putText(image, name, (left, bottom + 20), cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 1)


            if name == "Unknown" and saved_face_crop is None:
                saved_face_crop = rgb_image[top:bottom, left:right]


        cv2.imshow("Face Recognition", image)


        key = cv2.waitKey(1) & 0xFF


        if key == ord('s') and saved_face_crop is not None:
            face_filename = os.path.join(faces_folder, f"face_{len(known_face_names) + 1}.jpg")
            cv2.imwrite(face_filename, cv2.cvtColor(saved_face_crop, cv2.COLOR_RGB2BGR))
            print(f"Face saved as {face_filename}")
            load_known_faces()


        if key == ord('q'):
            print("Exiting...")
            cv2.destroyAllWindows()
            break


        gc.collect()


    except Exception as e:
        print(f"Error: {e}")
        continue
