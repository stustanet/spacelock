import json
import re
from datetime import datetime

from flask import Flask, render_template, request, redirect, url_for, flash
from flask.views import View, MethodView
from flask_login import LoginManager, current_user, login_user, logout_user, login_required
from flask_qrcode import QRcode
from flask_wtf import CSRFProtect

import settings
from authentication import User
from db import gen_token, modify_user, add_user, enable_user, disable_user, list_users, grant_access, del_user

app = Flask(__name__)
app.secret_key = settings.SECRET_KEY

QRcode(app)
csrf = CSRFProtect(app)

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
        user = User.login(key=request.form.get('secret_key'))
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


@app.route('/access-request', methods=['GET', 'POST'])
def access_request():
    data = {}
    if request.method == 'POST':
        req_id, key = add_user()

        data['req_id'] = req_id
        data['key'] = key
        data['grant_access_link'] = url_for('advanced', req_id=req_id, _external=True)
        data['is_request'] = True
    elif request.method == 'GET':
        data['is_request'] = False

    return render_template('access_request.html', **data)


class AdvancedView(MethodView):
    decorators = [login_required]

    def get_template_name(self):
        return 'advanced.html'

    def render_template(self):
        return render_template(self.get_template_name(), **self.get_context())

    def get_context(self):
        users = list_users(current_user.key)
        return {
            'preselect': request.args.get('req_id'),
            'users': [
                {
                    'id': user[0],
                    'req_id': user[1],
                    'name': user[2],
                    'valid_from': user[4].astimezone(settings.TIMEZONE) if user[4] is not None else None,
                    'valid_to': user[4].astimezone(settings.TIMEZONE) if user[4] is not None else None,
                    'token_validity_time': user[6],
                    'active': user[7],
                    'usermod': user[8]
                }
                for user in users
            ]
        }

    def get(self):
        return self.render_template()

    def post(self):
        if 'action' not in request.args:
            return self.render_template()

        if request.args.get('action') == 'enable_user':
            res = enable_user(current_user.key, request.form.get('req_id'))

        elif request.args.get('action') == 'disable_user':
            res = disable_user(current_user.key, request.form.get('req_id'))

        elif request.args.get('action') == 'delete_user':
            res = del_user(current_user.key, request.form.get('req_id'))

        elif request.args.get('action') == 'modify_user':
            if re.match(r'^\d{2}/\d{2}/\d{4}$', request.form.get('valid_from_date')):
                date_pattern = '%m/%d/%Y'
            elif re.match(r'^\d{4}-\d{2}-\d{2}$', request.form.get('valid_from_date')):
                date_pattern = '%Y-%m-%d'
            else:
                flash('Unknown date format for valid_from/valid_to', category='danger')
                return self.render_template()

            valid_from = settings.TIMEZONE.localize(datetime.strptime(
                request.form.get('valid_from_date') + ' ' + request.form.get('valid_from_time'),
                f'{date_pattern} %H:%M:%S'))
            valid_to = settings.TIMEZONE.localize(datetime.strptime(
                request.form.get('valid_to_date') + ' ' + request.form.get('valid_to_time'),
                f'{date_pattern} %H:%M:%S'))

            usermod = request.form.get('usermod') == 'on'

            res = modify_user(current_user.key, request.form.get('req_id'), request.form.get('username'), valid_from,
                              valid_to, request.form.get('token_validity_time'), usermod)
            if res is None:
                flash('Error changing user', category='danger')
                return self.render_template()

        elif request.args.get('action') == 'grant_access':
            if re.match(r'^\d{2}/\d{2}/\d{4}$', request.form.get('valid_from_date')):
                date_pattern = '%m/%d/%Y'
            elif re.match(r'^\d{4}-\d{2}-\d{2}$', request.form.get('valid_from_date')):
                date_pattern = '%Y-%m-%d'
            else:
                flash('Unknown date format for valid_from/valid_to', category='danger')
                return self.render_template()

            valid_from = settings.TIMEZONE.localize(datetime.strptime(
                request.form.get('valid_from_date') + ' ' + request.form.get('valid_from_time'),
                f'{date_pattern} %H:%M:%S'))
            valid_to = settings.TIMEZONE.localize(datetime.strptime(
                request.form.get('valid_to_date') + ' ' + request.form.get('valid_to_time'),
                f'{date_pattern} %H:%M:%S'))

            res = grant_access(current_user.key, request.form.get('req_id'), request.form.get('username'), valid_from,
                               valid_to, request.form.get('token_validity_time'))

            if res is None:
                flash('Error granting access to user', category='danger')
                return self.render_template()
        else:
            flash(f'Unknown action {request.args.get("action")}', category='danger')
            return self.render_template()

        if res is None:
            flash('Error', category='danger')
            return self.render_template()

        return self.render_template()


@app.route('/', methods=['GET', 'POST'])
def index():
    if request.method == 'POST':
        token = gen_token(request.form.get('secret_key'))
        if token is None:
            return render_template('error.html', error='DENIED!!!')

        data = {
            'token': token,
            'token_url': settings.WIFI_SEND_URL + token
        }

        return render_template('access.html', **data)
    else:
        return render_template('index.html')


app.add_url_rule('/advanced', view_func=AdvancedView.as_view('advanced'))
