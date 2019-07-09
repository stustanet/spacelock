import json
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


@app.route('/api/token', methods=['POST'])
def api_token():
    try:
        jsn = request.get_json(force=True)
        if 'key' not in jsn:
            return app.response_class(
                response=json.dumps({'error': 'must contain "key" field'}),
                status=400,
                mimetype='application/json'
            )
        else:
            token = gen_token(jsn['key'])
            if token is not None:
                return app.response_class(
                    response=json.dumps({'token': token}),
                    status=200,
                    mimetype='application/json'
                )
            else:
                return app.response_class(
                    response=json.dumps({'error': 'invalid key - access denied'}),
                    status=403,
                    mimetype='application/json'
                )
    except:
        return app.response_class(
            response=json.dumps({'error': 'Invalid json'}),
            status=400,
            mimetype='application/json'
        )


@app.route('/advanced', methods=['GET'])
def advanced():
    data = {
        'users': [
            {
                'id': 1,
                'description': 'jotweh',
                'valid_from': '2019-08-08 11:12:20',
                'valid_to': '2019-08-08 11:12:20',
                'token_validity_time': 60 * 60 * 24 * 3,
                'active': True
            },
            {
                'id': 2,
                'description': 'jj',
                'valid_from': '2019-08-08 11:12:20',
                'valid_to': '2019-08-07 11:33:20',
                'token_validity_time': 60 * 60 * 24 * 1,
                'active': False
            }
        ]
    }

    return render_template('advanced.html', **data)


@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        token = gen_token(request.form['secret_key'])
        if token is None:
            return render_template('error.html', error='DENIED!!!')

        return render_template('access.html', token=token)
    else:
        return render_template('index.html')
