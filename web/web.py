import json
import os

from flask import Flask, render_template, request, redirect, url_for, flash
from flask_login import LoginManager, current_user, login_user, logout_user, login_required
from flask_qrcode import QRcode

from authentication import User
from db import gen_token


app = Flask(__name__)
app.secret_key = os.environ['FLASK_SECRET_KEY']

QRcode(app)

login_manager = LoginManager()
login_manager.init_app(app)
login_manager.login_view = "login"


@login_manager.user_loader
def load_user(user_id):
    return User.get(user_id)


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


@app.route('/login', methods=['GET', 'POST'])
def login():
    if current_user.is_authenticated:
        return redirect(url_for('advanced'))

    if request.method == 'POST':
        user = User(0)  # User.login(key=request.form.get('secret_key')).first()
        if user is None:
            flash('Invalid username or password')
            return redirect(url_for('login'))
        login_user(user)
        return redirect(url_for('advanced'))
    else:
        return render_template('login.html')


@app.route('/logout')
def logout():
    logout_user()
    return redirect(url_for('index'))


@app.route('/advanced', methods=['GET'])
# @login_required
def advanced():
    data = {
        'users': [
            {
                'id': 1,
                'name': 'jotweh',
                'valid_from': '2019-08-08 11:12:20',
                'valid_to': '2019-08-08 11:12:20',
                'token_validity_time': 60 * 60 * 24 * 3,
                'active': True
            },
            {
                'id': 2,
                'name': 'jj',
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
        token = gen_token(request.form.get('username'), request.form.get('secret_key'))
        if token is None:
            return render_template('error.html', error='DENIED!!!')

        return render_template('access.html', token=token)
    else:
        return render_template('index.html')
