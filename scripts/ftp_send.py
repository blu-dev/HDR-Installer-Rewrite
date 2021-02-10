from ftplib import FTP
import os, sys

consoleIP = ""
plugin_name = "HDR_Installer.nro"


#consoleIP = sys.argv[1]
if '.' not in consoleIP:
    print(sys.argv[0], "ERROR: missing console's IP!")
    sys.exit(-1)

consolePort = 5000

ftp = FTP()
print(f'Connecting to {consoleIP}... ', end='')
ftp.connect(consoleIP, consolePort)
ftp.login("anon", "anon")
print('Connected!')

binaryPath = "../" + plugin_name

print(binaryPath)
if os.path.isfile(binaryPath):
    sdPath = f'/switch/' + plugin_name
    print(f'Sending {sdPath}')
    ftp.storbinary(f'STOR {sdPath}', open(binaryPath, 'rb'))