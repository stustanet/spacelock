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
