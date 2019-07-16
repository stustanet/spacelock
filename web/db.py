import time
from datetime import datetime
from typing import Tuple, Optional, List

import psycopg2

import settings


class Database:
    def __init__(self, config):
        self.config = config
        self.conn = None
        self.connect()

        self.cur = None

    def __enter__(self):
        if self.closed():
            self.connect()

        self.cur = self.conn.cursor()
        return self.cur

    def __exit__(self, exc_type, exc_val, exc_tb):
        if not self.closed():
            self.cur.close()

    def __del__(self):
        if not self.closed():
            self.conn.close()

    def closed(self):
        if self.conn is None or self.conn.closed != 0:
            return True
        return False

    def connect(self):
        if self.conn is not None and self.conn.closed == 0:
            # already connected, do nothing
            return

        while self.closed():
            try:
                self.conn = psycopg2.connect(**self.config)
            except (psycopg2.OperationalError, psycopg2.InternalError):
                if self.conn is not None:
                    self.conn.close()
            time.sleep(1)

    def commit(self):
        if not self.closed():
            self.conn.commit()


database = Database(settings.DB_CONFIG)


def _exec_query(query, *params, fetchall=False, commit=False):
    try:
        with database as cursor:
            cursor.execute(query, params)

            if commit:
                database.commit()
            if not fetchall:
                return cursor.fetchone()
            else:
                return cursor.fetchall()
    except (psycopg2.DataError, psycopg2.IntegrityError):
        return None


def gen_token(key: str) -> Optional[str]:
    res = _exec_query('SELECT gen_token(%s)', key, commit=True)
    if res is None:
        return None
    return res[0]


def can_access(key: str, access_class: str) -> Optional[int]:
    res = _exec_query('SELECT can_access(%s, %s)', key, access_class)
    if res is None:
        return None
    return res[0]


def list_users(key: str) -> List:
    res = _exec_query(
        'SELECT id, reqid, name, granted_by, valid_from, valid_to, token_validity_time, active, usermod '
        'from user_list(%s)',
        key, fetchall=True)
    if res is None:
        return []
    return res


def add_user() -> Tuple[Optional[str], Optional[str]]:
    """
    create new user
    :return: Tuple consisting of registration code and secret key
    """
    res = _exec_query('SELECT * from user_add()', commit=True)
    if res is None:
        return None, None
    return res


def del_user(admin_key: str, req_id: str) -> Optional[str]:
    # TODO: implement such that is werks
    res = _exec_query('SELECT from user_del(%s, %s)', admin_key, req_id)

    if res is None:
        return None
    return res[0]


def modify_user(admin_key: str, req_id: str, username: str, valid_from: datetime, valid_to: datetime,
                token_validity_time: int, enable_usermod=False):
    res = _exec_query('SELECT user_mod(%s, %s, %s, %s, %s, %s, %s)', admin_key, req_id, username, valid_from, valid_to,
                      token_validity_time, enable_usermod, commit=True)
    if res is None:
        return None
    return res[0]


def grant_access(admin_key: str, req_id: str, username: str, valid_from: datetime, valid_to: datetime,
                 token_validity_time: int):
    res = _exec_query('SELECT user_grant_access(%s, %s, %s, %s, %s, %s)', admin_key, req_id, username, valid_from,
                      valid_to, token_validity_time, commit=True)

    if res is None:
        return None
    return res[0]


def enable_user(admin_key: str, req_id: str) -> Optional[str]:
    res = _exec_query('SELECT user_enable(%s, %s)', admin_key, req_id, commit=True)

    if res is None:
        return None
    return res[0]


def disable_user(admin_key: str, req_id: str) -> Optional[str]:
    res = _exec_query('SELECT user_disable(%s, %s)', admin_key, req_id, commit=True)

    if res is None:
        return None
    return res[0]
