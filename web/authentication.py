from flask_login import UserMixin

import db


class User(UserMixin):
    def __init__(self, user_id, name):
        self.id = user_id
        self.name = name

    def get_id(self):
        return self.id

    @staticmethod
    def login(name, key):
        user_id = db.can_access(name, key, 'usermod')
        if key is not None:
            return User(user_id, name)
        else:
            return None

    @staticmethod
    def get(user_id):
        user_id = db.get_user_by_id(user_id)
        if user_id is not None:
            return User(user_id)
        else:
            return None

