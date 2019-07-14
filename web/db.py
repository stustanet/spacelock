import os
from datetime import datetime
from typing import Tuple, Optional, List

import psycopg2


class Database:
    def __init__(self, config):
        self.config = config
        try:
            self.conn = psycopg2.connect(**self.config)
        except (Exception, psycopg2.DatabaseError) as error:
            if self.conn is not None:
                self.conn.close()
            raise error
        self.cur = None

    def __enter__(self):
        try:
            self.cur = self.conn.cursor()
        except (Exception, psycopg2.DatabaseError) as error:
            if self.cur is not None:
                self.cur.close()
            raise error

        if self.cur is not None:
            return self.cur
        else:
            return None

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.cur is not None:
            self.cur.close()

    def __del__(self):
        if self.conn is not None:
            self.conn.close()

    def commit(self):
        if self.conn is not None:
            self.conn.commit()


def db_config():
    return {
        'database': os.environ['SPACELOCK_DB_NAME'],
        'user': os.environ['SPACELOCK_DB_USER'],
        'password': os.environ['SPACELOCK_DB_PASS'],
        'host': os.environ.get('SPACELOCK_DB_HOST') or 'localhost',
        'port': int(os.environ.get('SPACELOCK_DB_PORT') or 5432)
    }


CONFIG = db_config()
database = Database(CONFIG)


def _exec_query(query, *params, fetchall=False, commit=False):
    with database as cursor:
        try:
            cursor.execute(query, params)

            if commit:
                database.commit()
            if not fetchall:
                return cursor.fetchone()
            else:
                return cursor.fetchall()
        except (Exception, psycopg2.DatabaseError) as error:
            cursor.close()


def gen_token(key: str) -> str:
    return _exec_query('SELECT gen_token(%s)', key, commit=True)[0]


def can_access(key: str, access_class: str) -> int:
    return _exec_query('SELECT can_access(%s, %s)', key, access_class)[0]


def list_users(key: str) -> List:
    return _exec_query(
        'SELECT id, reqid, name, granted_by, valid_from, valid_to, token_validity_time, active, usermod from user_list(%s)',
        key, fetchall=True)


def add_user() -> Tuple[str, str]:
    """
    create new user
    :return: Tuple consisting of registration code and secret key
    """
    return _exec_query('SELECT * from user_add()', commit=True)


def del_user(admin_key: str, req_id: str) -> Optional[str]:
    return _exec_query('SELECT from user_del(%s, %s)', admin_key, req_id)


def modify_user(admin_key: str, req_id: str, username: str, valid_from: datetime, valid_to: datetime,
                token_validity_time: int, enable_usermod=False):
    return _exec_query('SELECT user_mod(%s, %s, %s, %s, %s, %s, %s)', admin_key, req_id, username, valid_from, valid_to,
                       token_validity_time, enable_usermod, commit=True)


def grant_access(admin_key: str, req_id: str, username: str, valid_from: datetime, valid_to: datetime,
                 token_validity_time: int):
    return _exec_query('SELECT user_grant_access(%s, %s, %s, %s, %s, %s)', admin_key, req_id, username, valid_from,
                       valid_to, token_validity_time, commit=True)


def enable_user(admin_key: str, req_id: str) -> Optional[str]:
    return _exec_query('SELECT user_enable(%s, %s)', admin_key, req_id, commit=True)[0]


def disable_user(admin_key: str, req_id: str) -> Optional[str]:
    return _exec_query('SELECT user_disable(%s, %s)', admin_key, req_id, commit=True)[0]
