import os

import psycopg2


class Database:
    def __init__(self, config):
        self.config = config
        self.conn = None
        self.cur = None

    def __enter__(self):
        try:
            self.conn = psycopg2.connect(**self.config)
            self.cur = self.conn.cursor()
        except (Exception, psycopg2.DatabaseError) as error:
            if self.cur is not None:
                self.cur.close()
            if self.conn is not None:
                self.conn.close()
            raise error

        if self.cur is not None:
            return self.cur
        else:
            return None

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.cur is not None:
            self.cur.close()

        if self.conn is not None:
            self.conn.close()


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


def get_user_by_key(key):
    with Database(db_config()) as db:
        db.execute('SELECT get_user_by_key(%s)', (key,))
        return db.fetchone()


def get_user_by_id(user_id):
    with Database(db_config()) as db:
        db.execute('SELECT get_user_by_id(%s)', (user_id,))
        return db.fetchone()
