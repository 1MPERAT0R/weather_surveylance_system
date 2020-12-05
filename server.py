# Author: Zac Turner

import socket
import threading
from datetime import datetime
import csv
import os

port = 25425

listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # ipv4, tcp
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) # prevents the socket from staying in use after the server is shut off
listener.bind(('', port))
listener.listen(5)

# Saves any received weather data to a file
def recordWeather(clientSocket, clientAddress, mac, Date, Time):
    try:
        message = clientSocket.recv(65535).decode('utf-8')
    except:
        return
    
    print(str(clientAddress[0]) + " sent weather data")
    
    splitMessage = message.split()
        
    humidity = splitMessage[0]
    pressure = splitMessage[1]
    temperature = splitMessage[2]
    
    home = os.getcwd()
    todayFolder = home + "/" + str(Date)
    deviceFolder = todayFolder + "/" + "weather_" + mac
    if os.path.exists(todayFolder) == False:
        try:
            os.mkdir(todayFolder)
            os.mkdir(deviceFolder)
        except OSError:
            print("failed to create folder, no data was saved");
            return
    else:
        if os.path.exists(deviceFolder) == False:
            try:
                os.mkdir(deviceFolder)
            except OSError:
                print("failed to create folder, no data was saved");
                return
            
    with open(deviceFolder + "/" + "weatherData.csv", 'a', newline='') as f:
        writer = csv.writer(f)
        writer.writerow([Time, humidity, pressure, temperature])
        f.close()
        
# Saves any received Images to a file
def recordImage(clientSocket, clientAddress, mac, Date, Time):
    home = os.getcwd()
    todayFolder = home + "/" + str(Date)
    deviceFolder = todayFolder + "/" + "images_" + mac
    if os.path.exists(todayFolder) == False:
        try:
            os.mkdir(todayFolder)
            os.mkdir(deviceFolder)
        except OSError:
            print("failed to create folder, no image was saved");
            return
    else:
        if os.path.exists(deviceFolder) == False:
            try:
                os.mkdir(deviceFolder)
            except OSError:
                print("failed to create folder, no image was saved");
                return
            
    f = open(deviceFolder + "/" + str(Time) + ".jpg", 'ab')
    try:
        message = clientSocket.recv(65535)
        f.write(message)
        length = len(message)
        while message[(length - 2):length] != b'\xff\xd9':
            message = clientSocket.recv(65535)
            f.write(message)
            length = len(message)
        print(str(clientAddress[0]) + " sent an image")
        f.close()
    except:
        print("something broke when receiving the image")
        f.close()
        return

# Determines the contentType for an HTTP request, text is returned if there is no requested type
def contentType(fileName):
    fileType = os.path.splitext(fileName)[1]
    if fileType == ".html":
        return "text/html"
    if fileType == ".jpg":
        return "image/jpeg"
    if fileType == '': # serves the html page by default
        return "text/html"
    
    raise Exception("undefined fileType")

# Interprets HTTP requests and sends the requested file if possible.
def httpHandler(socket, address, Date, message):
    CRLF = "\r\n"
    
    splitMessage = message.splitlines()
    requestLine = splitMessage[0]
    tokens = requestLine.split()
    fileName = tokens[1] # possibly add [1:] ??
    if fileName[0] == '/':
        fileName = fileName[1:]
    
    try:
        statusline = "HTTP/1.1 200 OK" + CRLF
        contentTypeLine = "Content-type: " + contentType(fileName) + CRLF
        if fileName == '': # if the filename is empty then the data.html page will be sent
            HTML_builder(Date) # Build the HTML page for the client
            file = open('data.html', 'r')
            entityBody = file.read().encode('utf-8')
        else:
            file = open(fileName, 'rb')
            entityBody = file.read()
        file.close()
    except:
        statusline = "HTTP/1.1 404 Not Found" + CRLF
        contentTypeLine = "Content-type: text/html" + CRLF
        entityBody = "<HTML><HEAD><TITLE>Not Found</TITLE></HEAD><BODY>Not Found</BODY></HTML>"
    
    socket.send(statusline.encode('utf-8'))
    socket.send(contentTypeLine.encode('utf-8'))
    socket.send(CRLF.encode('utf-8'))
    socket.send(entityBody)

# Builds an HTML webpage with the most recently obtained data
def HTML_builder(Date):
    try:
        directory = os.listdir(str(Date))
        weatherFolders = []
        imageFolders = []
        for x in directory:
            if "weather" in x:
                weatherFolders.append(x)
            elif "images" in x:
                imageFolders.append(x)
        
        with open("data.html", 'w') as f:
            f.write("<!DOCTYPE html>\n")
            f.write("<html>\n")
            f.write("<head>\n")
            f.write("<title>Statistics and Images</title>\n")
            f.write("</head>\n")
            f.write("<body>\n")
            
            if len(weatherFolders) > 0:
                f.write("<h1>Weather Statistics</h1>\n")
                for x in weatherFolders:
                    with open(str(Date) + "/" + x + "/" + "weatherData.csv") as c:
                        reader = csv.reader(c, delimiter=',')
                        for row in reader: # The most recent data will be on the last row
                            data = row
                        c.close()
                    f.write("<p>Device: " + x[8:] + "</p>\n")
                    f.write("<p>Time Recorded: " + data[0][0:5] + "</p>\n")
                    f.write("<p>Air Pressure: " + data[2] + "</p>\n")
                    f.write("<p>Temperature: " + data[3] + "</p>\n")
                    f.write("<p>Humidity: " + data[1] + "</p>\n")
                    f.write("<br>\n")
            else:
                f.write("<h1>No Weather Data Available</h1>\n")
                f.write("<br>\n")
            
            if len(imageFolders) > 0:
                f.write("<h1>Images</h1>\n")
                for x in imageFolders:
                    newestImage = os.listdir(str(Date) + "/" + x)[-1]
                    f.write("<p>Camera: " + x[7:] + "</p>\n")
                    f.write("<p>Time Recorded: " + newestImage[0:5] + "</p>\n")
                    f.write("<img src=\"" + str(Date) + "/" + x + "/" + newestImage + "\">\n<br><br>\n")
            else:
                f.write("<h1>No Images Available</h1>")
                f.write("<br>\n")
            
            f.write("</body>\n")
            f.write("</html>\n")
            
            f.close()
    except:
        with open("data.html", 'w') as f:
            f.write("<!DOCTYPE html>\n")
            f.write("<html>\n")
            f.write("<head>\n")
            f.write("<title>No data today</title>\n")
            f.write("</head>\n")
            f.write("<body>\n")
            f.write("<h1>No data recorded today, try again later</h1>\n")
            f.write("</body>\n")
            f.write("</html>\n")
            
            f.close()

# Calls either recordWeather, recordImage, or httpHandler depending on what the first message from the client contains
def clientHandler(clientSocket, clientAddress):
    DateTime = datetime.now()
    Date = datetime.date(DateTime)
    Time = datetime.time(DateTime)
        
    message = clientSocket.recv(65535)
    message = message.decode('utf-8')
    if message[0:4] == "info":
        if message[5:12] == "weather":
            mac = message[13:]
            recordWeather(clientSocket, clientAddress, mac, Date, Time)
        elif message[5:11] == "camera":
            mac = message[12:]
            recordImage(clientSocket, clientAddress, mac, Date, Time)
        else:
            print("unknown client type")
    elif message[0:3] == "GET":
        httpHandler(clientSocket, clientAddress, Date, message)
    else:
        print("unknown client type")
    
    # close the socket when finished
    print(str(clientAddress[0]) + " has disconnected from port " + str(clientAddress[1]))
    clientSocket.close()   




print("Server started, ctrl + c to shutdown")
print("listening")

# Main server loop, runs indefinitely unless interrupted
while True:
    try:
        (clientSocket, clientAddress) = listener.accept()
        print(str(clientAddress[0]) + " connected on port " + str(clientAddress[1]))
        clientThread = threading.Thread(target=clientHandler, args=(clientSocket, clientAddress), daemon=True)
        clientThread.start()
    except KeyboardInterrupt:
        listener.close()
        print("\nServer shutting down")
        break