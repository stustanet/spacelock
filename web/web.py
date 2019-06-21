import os
from flask import Flask, render_template, request

from utils import Database


app = Flask(__name__)


def db_config():
    return {
        'database': os.environ['SPACELOCK_DB_NAME'],
        'user': os.environ['SPACELOCK_DB_USER'],
        'password': os.environ['SPACELOCK_DB_PASS'],
        'host': os.environ['SPACELOCK_DB_HOST'],
        'port': int(os.environ['SPACELOCK_DB_PORT'])
    }


def gen_token(key):
    with Database(db_config()) as db:
        db.execute('SELECT gen_token(%s)', (key,))
        return db.fetchone()[0]


@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        token = gen_token(request.form['secret_key'])
        print(token)
        if token is None:
            return render_template('error.html', error='DENIED!!!')

        return render_template('access.html', token=token)
    else:
        return render_template('index.html')
