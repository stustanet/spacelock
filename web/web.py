import os
from flask import Flask, render_template, request
from flask_qrcode import QRcode

from utils import Database


app = Flask(__name__)
QRcode(app)


def db_config():
    return {
        'database': os.environ['SPACELOCK_DB_NAME'],
        'user': os.environ['SPACELOCK_DB_USER'],
        'password': os.environ['SPACELOCK_DB_PASS'],
        'host': os.environ.get('SPACELOCK_DB_HOST') or 'localhost',
        'port': int(os.environ.get('SPACELOCK_DB_PORT') or 5432)
    }


def gen_token(key):
    with Database(db_config()) as db:
        db.execute('SELECT gen_token(%s)', (key,))
        return db.fetchone()[0]


@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        token = gen_token(request.form['secret_key'])
        if token is None:
            return render_template('error.html', error='DENIED!!!')

        return render_template('access.html', token=token)
    else:
        return render_template('index.html')
