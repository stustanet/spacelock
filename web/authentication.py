from flask_login import UserMixin

import db


class User(UserMixin):
    def __init__(self, user_id):
        self.id = user_id

    def get_id(self):
        return self.id

    @staticmethod
    def login(key):
        user_id = db.get_user_by_key(key)
        if key is not None:
            return User(user_id)
        else:
            return None

    @staticmethod
    def get(user_id):
        user_id = db.get_user_by_id(user_id)
        if user_id is not None:
            return User(user_id)
        else:
            return None

