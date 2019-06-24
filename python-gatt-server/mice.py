import requests

def send_token(token):
    print("er")
    r = requests.get(f"http://127.0.0.1:8000/send/{token}")
    print("token sent")
    # import subprocess
    # subprocess.run(["wget", "-O", "/dev/null", f"http://127.0.0.1:8000/send/{token}"])
