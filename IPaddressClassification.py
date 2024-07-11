import cv2  # opencv
import urllib.request  # para abrir y leer URL
import numpy as np
import os

# Programa de clasificación de objetos para video en dirección IP

url = 'http://192.168.18.119/cam-hi.jpg'
winName = 'ESP32 CAMERA'
cv2.namedWindow(winName, cv2.WINDOW_AUTOSIZE)
fruitWinName = 'Fruta Detectada'

# Asegúrate de ajustar la ruta a la ubicación real de tus archivos
base_path = r'C:\Users\Usuario\Desktop\Proyectos Arduino\yfy\esp32cam-IA-main'
classFile = os.path.join(base_path, 'coco.names')
configPath = os.path.join(base_path, 'ssd_mobilenet_v3_large_coco_2020_01_14.pbtxt')
weightsPath = os.path.join(base_path, 'frozen_inference_graph.pb')

classNames = []
with open(classFile, 'rt') as f:
    classNames = f.read().rstrip('\n').split('\n')

net = cv2.dnn.DetectionModel(weightsPath, configPath)
net.setInputSize(320, 320)
net.setInputScale(1.0 / 127.5)
net.setInputMean((127.5, 127.5, 127.5))
net.setInputSwapRB(True)

# Lista de frutas a detectar con niveles de desastre
frutas_niveles = {
    "apple": "nivel de desastre detectado: alto",
    "banana": "nivel de desastre detectado: medio",
    "orange": "nivel de desastre detectado: bajo"
}

# Archivo para registrar las frutas detectadas
registro_frutas = os.path.join(base_path, "frutas_detectadas.txt")

while True:
    try:
        imgResponse = urllib.request.urlopen(url)  # abrimos el URL
        imgNp = np.array(bytearray(imgResponse.read()), dtype=np.uint8)
        img = cv2.imdecode(imgNp, -1)  # decodificamos

        detections = net.detect(img, confThreshold=0.5)
        classIds, confs, bbox = detections if len(detections) == 3 else (None, None, None)
        print(classIds, bbox)

        if classIds is not None and len(classIds) > 0:
            for classId, confidence, box in zip(classIds.flatten(), confs.flatten(), bbox):
                label = classNames[classId - 1]
                if label in frutas_niveles:
                    nivel_desastre = frutas_niveles[label]
                    cv2.rectangle(img, box, color=(0, 255, 0), thickness=3)  # mostramos en rectángulo lo que se encuentra
                    cv2.putText(img, label, (box[0] + 10, box[1] + 30), cv2.FONT_HERSHEY_COMPLEX, 1, (0, 255, 0), 2)

                    # Mostrar el nivel de desastre en una ventana aparte
                    fruit_img = np.zeros((300, 600, 3), dtype=np.uint8)  # tamaño de la ventana ajustado
                    text_size = cv2.getTextSize(nivel_desastre, cv2.FONT_HERSHEY_SIMPLEX, 1, 2)[0]
                    text_x = (fruit_img.shape[1] - text_size[0]) // 2
                    text_y = (fruit_img.shape[0] + text_size[1]) // 2
                    cv2.putText(fruit_img, nivel_desastre, (text_x, text_y), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                    cv2.imshow(fruitWinName, fruit_img)

                    # Registrar la fruta detectada en el archivo
                    with open(registro_frutas, 'a') as registro:
                        registro.write(f"{label}: {nivel_desastre}\n")

        cv2.imshow(winName, img)  # mostramos la imagen

        # Esperamos a que se presione ESC para terminar el programa
        tecla = cv2.waitKey(5) & 0xFF
        if tecla == 27:
            break
    except Exception as e:
        print(f"Error: {e}")
        break

cv2.destroyAllWindows()
