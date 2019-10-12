import urllib.request
import os
import sys
argvs = sys.argv
import ssl
ssl._create_default_https_context = ssl._create_unverified_context

if len(argvs) < 2:
    print('please specify a destination path.')
    sys.exit()

os.chdir(os.path.dirname(os.path.abspath(__file__)))

if not os.path.exists(argvs[1] + '/bgm1.ogg'):
    urllib.request.urlretrieve('https://github.com/altseed/OpenSoundMixer2/releases/download/TestData/bgm1.ogg', argvs[1] + '/bgm1.ogg')

if not os.path.exists(argvs[1] + '/se1.wav'):
    urllib.request.urlretrieve('https://github.com/altseed/OpenSoundMixer2/releases/download/TestData/se1.wav', argvs[1] + '/se1.wav')



        
