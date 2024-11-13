import sys
from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QVBoxLayout, QHBoxLayout, QLabel, \
    QMessageBox, QTextEdit, QComboBox, QFileDialog
from PyQt5.QtGui import QPalette, QColor, QFont
from PyQt5.QtCore import Qt
import serial.tools.list_ports
import serial
import time

class STM32F4FlashingTool(QWidget):
    CMD_GET_VERSION = 0x01
    CMD_GET_HELP = 0x02
    CMD_GET_ID = 0x03
    CMD_FLASH_APP = 0x04
    CMD_FLASH_ERASE = 0x05
    CMD_MEM_READ = 0x06
    CMD_OTP_READ = 0x07
    CMD_EXIT = 0x08

    def __init__(self):
        super().__init__()

        self.serialPort = None
        self.timeoutSeconds = 5
        self.tempFilePath = "temp.hex"
        self.filePath = ""

        self.initUI()

    def initUI(self):
        layout = QVBoxLayout()
        self.setDarkMode()

        title_label = QLabel('<font color="#2a82da">STM32F401CC</font> <b><font color="#ffffff">Flash Tool</font></b>',
                             self)
        title_font = QFont("Arial", 18, QFont.Bold)
        title_label.setFont(title_font)
        title_label.setAlignment(Qt.AlignCenter)

        self.serialComboBox = QComboBox(self)
        self.updateSerialPorts()

        serial_label = QLabel('Please select the COM port:', self)

        refresh_button = QPushButton('Refresh COM Ports', self)
        refresh_button.clicked.connect(self.updateSerialPorts)
        layout.addWidget(title_label)
        layout.addWidget(serial_label)
        layout.addWidget(self.serialComboBox)
        layout.addWidget(refresh_button)

        # Information Operations
        info_label = QLabel("<b>Information Operations</b>", self)
        info_label.setAlignment(Qt.AlignLeft)
        info_label.setStyleSheet("color: #2a82da")  # Set the color as needed
        layout.addWidget(info_label)

        info_buttons = [
            QPushButton('Request Firmware Version', self),
            QPushButton('Request Information', self),
            QPushButton('Retrieve Chip Identification Number', self)
        ]



        for button in info_buttons:
            button.clicked.connect(self.runCommand)
            layout.addWidget(button)

        # Memory Operations
        memory_label = QLabel("<b>Memory Operations</b>", self)
        memory_label.setAlignment(Qt.AlignLeft)
        memory_label.setStyleSheet("color: #2a82da")  # Set the color as needed
        layout.addWidget(memory_label)

        memory_buttons = [
            QPushButton('Flash New Application', self),
            QPushButton('Flash Memory Erase', self),
            QPushButton('Retrieve Data from Memory', self),
            QPushButton('OTP Memory Read', self)
        ]

        for button in memory_buttons:
            button.clicked.connect(self.runCommand)
            layout.addWidget(button)


        self.logBox = QTextEdit(self)
        self.logBox.setReadOnly(True)

        layout.addWidget(title_label)
        layout.addWidget(serial_label)
        layout.addWidget(self.serialComboBox)
        layout.addWidget(refresh_button)
        layout.addSpacing(20)  # Adjust the spacing as needed
        layout.addWidget(self.logBox)

        self.setLayout(layout)

        self.setGeometry(300, 300, 800, 800)
        self.setWindowTitle('STM32F401CC Flash Tool')

        self.show()

    def openSerialConnection(self):
        port_name = self.serialComboBox.currentText()
        if not port_name:
            QMessageBox.information(self, "Error", "Please select a serial port.")
            return

        self.serialPort = self.openSerialPort(port_name)
        if self.serialPort:
            #QMessageBox.information(self, "Success", f"Opened serial port: {port_name}")
            pass
        else:
            QMessageBox.information(self, "Error", f"Failed to open serial port: {port_name}")

    def openSerialPort(self, port_name):
        try:
            ser = serial.Serial(port_name, baudrate=9600, timeout=5)
            print(f"Opened serial port: {port_name}")
            return ser
        except Exception as e:
            print(f"Failed to open serial port {port_name}. Error: {e}")
            return None

    def updateSerialPorts(self):
        ports = [port.device for port in serial.tools.list_ports.comports()]
        self.serialComboBox.clear()
        self.serialComboBox.addItems(ports)

    def setDarkMode(self):
        # Set Dark Mode for the application
        app.setStyle("Fusion")
        palette = QPalette()

        # Set background and text color for message boxes
        palette.setColor(QPalette.Window, QColor(53, 53, 53))
        palette.setColor(QPalette.WindowText, QColor(255, 255, 255))
        palette.setColor(QPalette.Button, QColor(53, 53, 53))
        palette.setColor(QPalette.ButtonText, QColor(255, 255, 255))
        palette.setColor(QPalette.Highlight, QColor(42, 130, 218))
        palette.setColor(QPalette.HighlightedText, QColor(0, 0, 0))

        # Set background and text color for other widgets
        palette.setColor(QPalette.Base, QColor(25, 25, 25))
        palette.setColor(QPalette.AlternateBase, QColor(53, 53, 53))
        palette.setColor(QPalette.ToolTipBase, QColor(255, 255, 255))
        palette.setColor(QPalette.ToolTipText, QColor(0, 0, 0))
        palette.setColor(QPalette.Text, QColor(255, 255, 255))
        palette.setColor(QPalette.Button, QColor(53, 53, 53))
        palette.setColor(QPalette.ButtonText, QColor(255, 255, 255))
        palette.setColor(QPalette.Window, QColor(53, 53, 53))
        palette.setColor(QPalette.WindowText, QColor(255, 255, 255))

        self.setPalette(palette)

        self.setStyleSheet(
            "QPushButton { background-color: #2a82da; color: white; border: 1px solid #2a82da; padding: 5px; }"
            "QPushButton:hover { background-color: #007bff; }"
            "QComboBox { background-color: #2a82da; color: white; border: 1px solid #2a82da; padding: 5px; }"
            "QComboBox:hover { background-color: #007bff; }"
            "QTextEdit { background-color: #333; color: white; border: 1px solid #2a82da; padding: 5px; }"
            "QLabel { color: gray; }")

    def runCommand(self):
        # if self.serialPort:
        button_text = self.sender().text()
        self.openSerialConnection()

        if button_text == 'Request Firmware Version':
            self.cblGetVerCmd()
        elif button_text == 'Request Information':
            self.cblGetHelpCmd()
        elif button_text == 'Retrieve Chip Identification Number':
            self.cblGetCidCmd()
        elif button_text == 'Flash New Application':
            self.cblMemWriteCmd()
        elif button_text == 'Flash Memory Erase':
            self.cblFlashEraseCmd()
        elif button_text == 'Retrieve Data from Memory':
            self.cblMemReadCmd()
        elif button_text == 'OTP Memory Read':
            self.cblOtpReadCmd()

        self.closeEvent()
        # else:
        #     print(f"Failed to open serial port")

    def cblGetHelpCmd(self):
        if self.serialPort:
            QMessageBox.information(self, "Request for Information", "Requesting information or assistance.")
        else:
            QMessageBox.information(self, "Error", "Serial port is not open. Please open a serial connection.")

    def cblGetCidCmd(self):
        if self.serialPort:
            QMessageBox.information(self, "Retrieve Chip ID", "Retrieving the identification number of the chip.")
        else:
            QMessageBox.information(self, "Error", "Serial port is not open. Please open a serial connection.")

    def cblFlashEraseCmd(self):
        if self.serialPort:
            QMessageBox.information(self, "Flash Memory Erase", "Clearing the content of the flash memory.")
        else:
            QMessageBox.information(self, "Error", "Serial port is not open. Please open a serial connection.")

    def cblMemWriteCmd(self):
        if (self.selectHexFile() == None):
            return
        try:
            self.logBox.append(f"Start to flash application: {self.filePath}")
            if not self.check_hex_file_validity(self.filePath):
                raise Exception("Invalid hex file format. Aborting CBL_MEM_WRITE_CMD.")

            if not self.serialPort:
                raise Exception("Serial port is not open. Please open a serial connection.")

            # Command to store data in the device's memory
            if not self.check_hex_file_validity(self.filePath):
                QMessageBox.information(self, "Error", "Invalid hex file format. Aborting CBL_MEM_WRITE_CMD.")
                return

            self.remove_colon_from_hex_file(self.filePath, self.tempFilePath)

            with open(self.tempFilePath, 'r') as file:
                lines = file.readlines()

            recordsPerIteration = 30
            totalRecords = len(lines)
            totalCharacters = sum(len(record.strip()) for record in lines)
            hexTotalCharacters = hex(totalCharacters)[2:].upper()
            iterationCount = 0

            while iterationCount * recordsPerIteration < totalRecords:
                startIndex = iterationCount * recordsPerIteration
                endIndex = min(startIndex + recordsPerIteration, totalRecords)
                records = lines[startIndex:endIndex]

                bufferRecordsNo = endIndex - startIndex
                bufferRecordsByteArray = self.valueToHex2ByteArray(bufferRecordsNo)

                firstIterationBytesLength = sum(len(record.strip()) + 1 for record in records)
                firstIterationByteArray = self.valueToHex2ByteArray(firstIterationBytesLength)

                commandBytes = bytearray([firstIterationByteArray[0], firstIterationByteArray[1], self.CMD_FLASH_APP])
                self.sendData(commandBytes)

                if iterationCount == 0:
                    acknowledgment = self.checkAcknowledgement(self.CMD_FLASH_APP)
                    if acknowledgment:
                        pass
                    else:
                        self.logBox.append("Unexpected acknowledgment or timeout. Exiting loop.")
                        break

                while True:
                    nextStartIndex = (iterationCount + 1) * recordsPerIteration
                    nextEndIndex = min(nextStartIndex + recordsPerIteration, totalRecords)
                    nextRecords = lines[nextStartIndex:nextEndIndex]
                    nextIterationBytesLength = sum(len(record.strip()) + 1 for record in nextRecords)
                    nextIterationByteArray = self.valueToHex2ByteArray(nextIterationBytesLength)

                    if nextIterationBytesLength == 0:
                        print(f"Sending Packet No.{(iterationCount + 1)}")
                        self.logBox.append(f"Sending Packet No.{(iterationCount + 1)}")
                        modifiedString = ''.join(
                            [chr(byte) for byte in [bufferRecordsByteArray[0], 0, 0]] + [record.strip() + '\n' for
                                                                                         record in records])
                    else:
                        print(f"Sending Packet No.{(iterationCount + 1)}")
                        self.logBox.append(f"Sending Packet No.{(iterationCount + 1)}")
                        modifiedString = ''.join([chr(byte) for byte in
                                                  [bufferRecordsByteArray[0], nextIterationByteArray[0],
                                                   nextIterationByteArray[1]]] + [record.strip() + '\n' for record in
                                                                                  records])

                    if iterationCount == (totalRecords // recordsPerIteration):
                        modifiedString = '\x00' + modifiedString
                    else:
                        modifiedString = '\x01' + modifiedString

                    chunksByteArray = modifiedString.encode('utf-8')
                    time.sleep(0.7)
                    self.flush()
                    self.sendData(chunksByteArray)

                    acknowledgment = self.checkAcknowledgement(self.CMD_FLASH_APP)

                    if acknowledgment:
                        iterationCount += 1
                        break
                    else:
                        self.logBox.append("Error, Unexpected acknowledgment: {}".format(acknowledgment))
                        break

            QMessageBox.information(self, 'Flashing done', "Your application has been flashed")
            message = "<font color='green'>Application flashed successfully.</font>"
            self.logBox.append(message)

        except Exception as e:
            self.logBox.append(f"Error: {e}")

    def checkAcknowledgement(self, expectedValue):
        start_time = time.time()
        while self.serialPort.inWaiting() < 1:
            if time.time() - start_time > self.timeoutSeconds:
                self.logBox.append("Timeout reached while waiting for acknowledgment.")
                return False

        data = self.readData(self.serialPort.inWaiting())
        data = data[0]
        return data == chr(expectedValue)

    def cblMemReadCmd(self):
        if self.serialPort:
            QMessageBox.information(self, "Memory Read", "Performing CBL_MEM_READ_CMD: Retrieving data from the device's memory.")
        else:
            QMessageBox.information(self, "Error", "Serial port is not open. Please open a serial connection.")

    def selectHexFile(self):
        options = QFileDialog.Options()
        options |= QFileDialog.DontUseNativeDialog
        file_path, _ = QFileDialog.getOpenFileName(self, "Select Hex File", "", "Hex Files (*.hex);;All Files (*)",
                                                   options=options)

        if file_path:
            self.filePath = file_path
            self.logBox.append(f"Selected hex file: {file_path}")
            return file_path
        else:
            self.logBox.append("File selection canceled.")
            return None

    def cblOtpReadCmd(self):
        if self.serialPort:
            QMessageBox.information(self, "OTP Memory Read", "Reading data from One-Time Programmable (OTP) memory.")
        else:
            QMessageBox.information(self, "Error", "Serial port is not open. Please open a serial connection.")

    def valueToHex2ByteArray(self, value):
        hexValue = hex(value)[2:].upper()
        byteArray = [int(hexValue[i:i + 2], 16) for i in range(0, len(hexValue), 2)]
        return byteArray

    def cblGetVerCmd(self):
        if self.serialPort:
            commandBytes = bytearray([0x00, 0x00, self.CMD_GET_VERSION])
            self.sendData(commandBytes)
            version = self.readData(27)
            self.logBox.append(version)
            QMessageBox.information(self, "Firmware Version", version)


        else:
            QMessageBox.information(self, "Error", "Serial port is not open. Please open a serial connection.")

    def sendData(self, command):
        try:
            if self.serialPort:
                self.serialPort.flushOutput()
                self.serialPort.write(command)
            else:
                print("Serial port is not open.")
        except Exception as e:
            print(f"Failed to send the message on port. Error: {e}")

    def readData(self, dataNum):
        try:
            if self.serialPort:
                data = self.serialPort.read(dataNum).decode('utf-8')
                return data
            else:
                print("Serial port is not open.")
        except Exception as e:
            print(f"Failed to read data from serial port. Error: {e}")

    def remove_colon_from_hex_file(self, input_file, output_file):
        with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
            for line in infile:
                cleaned_line = line.lstrip(':')
                outfile.write(cleaned_line)

    def check_hex_file_validity(self, file_path):
        with open(file_path, 'r') as file:
            lines = file.readlines()

        for line in lines:
            stripped_line = line.rstrip('\n')
            if not stripped_line.startswith(':') or len(stripped_line) < 11 or (len(stripped_line) - 1) % 2 != 0:
                return False
        return True

    def flush(self):
        if self.serialPort:
            self.serialPort.flushOutput()
            self.serialPort.flushInput()
        else:
            print("Serial port is not open.")

    def closeEvent(self):
        if self.serialPort and self.serialPort.is_open:
            self.serialPort.close()

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = STM32F4FlashingTool()
    sys.exit(app.exec_())
