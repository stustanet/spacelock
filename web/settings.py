import os

import pytz

TIMEZONE = pytz.timezone('Europe/Berlin')

SECRET_KEY = os.environ['FLASK_SECRET_KEY']

DB_CONFIG = {
    'database': os.environ['SPACELOCK_DB_NAME'],
    'user': os.environ['SPACELOCK_DB_USER'],
    'password': os.environ.get('SPACELOCK_DB_PASS'),
    'host': os.environ.get('SPACELOCK_DB_HOST'),
    'port': int(os.environ.get('SPACELOCK_DB_PORT') or 5432)
}
