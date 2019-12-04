from flask_login import UserMixin

import db


class User(UserMixin):
    def __init__(self, user_id, key):
        self.id = user_id
        self.key = key

    def get_id(self):
        return self.key

    @staticmethod
    def login(key):
        user_id = db.can_access(key, 'usermod')
        if user_id is not None:
            return User(user_id, key)
        else:
            return None

    @staticmethod
    def get(key):
        user_id = db.can_access(key, 'usermod')
        if user_id is not None:
            return User(user_id, key)
        else:
            return None
