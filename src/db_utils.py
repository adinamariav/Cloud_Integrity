import sqlite3
from sqlite3 import Error

class DB_Connector(object):
    def __init(self):
        self.DB_CURSOR     = None
        self.DB_CONNECTION = None
    try:
        DB_CONNECTION = sqlite3.connect('../data/bosc.sqlite')
        DB_CURSOR = DB_CONNECTION.cursor()
    except Error as e:
        print(e)

    def validate_bosc_table(self):
        query = "CREATE TABLE IF NOT EXISTS BoSC (BAGS TEXT PRIMARY KEY);"
        try:
            self.DB_CURSOR.execute(query)
            self.DB_CONNECTION.commit()
        except Exception as e:
            print(str(e)," during database validation")

    def check_existence(self, data):
        query = "SELECT * FROM BoSC WHERE BAGS = ?"
        try:
            self.DB_CURSOR.execute(query,(data,))
        except Exception as e:
            return False
        
        return True

    def insert_bag__if_nonexistent(self, data):
        query = "SELECT * FROM BoSC WHERE BAGS = ?"
        try:
            self.DB_CURSOR.execute(query,(data,))
            db_results = self.DB_CURSOR.fetchall()
        except Exception as e:
            db_results = []
        if db_results == []:
            insert_query = "INSERT INTO BoSC(BagS) VALUES (?)"
            self.DB_CURSOR.execute(insert_query,(data,))
            self.DB_CONNECTION.commit()



if __name__ == '__main__':
    connector = DB_Connector()