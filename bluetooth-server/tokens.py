import requests


def send_token(token):
    return requests.get(f"http://127.0.0.1:8000/send/{token}")
