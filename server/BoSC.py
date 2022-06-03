import pandas as pd
from db_utils import DB_Connector
import sys

class BoSC_Creator(object):

    def __init__(self, windowsize):
        
        self.WINDOW_SIZE = windowsize
        self.db_connector = None
        self.syscall_LUT   = {}
        self.sliding_window= []
        self.anomalies = 0

        self.DB_CONNECTION = DB_Connector()

    def load_lookup_table(self):
        with open("../data/lut.csv") as csv_file:
            df = pd.read_csv(csv_file)

        df_stripped = pd.DataFrame(df['Syscall'])
        df_stripped['index'] = range(0, len(df))
        self.syscall_LUT = pd.Series(df_stripped['index'].values,index=df_stripped.Syscall).to_dict()        

    def create_BoSC(self):
        bag_size = len(self.syscall_LUT)
        bag = [0] * (bag_size + 1)
        
        for syscall in self.sliding_window:
            try:
                index = self.syscall_LUT[syscall]
                temp = bag[index]
                temp += 1
                bag[index] = temp
            except KeyError:
                temp = bag[-1]
                temp += 1
                bag[-1] = temp

        return str(bag)

    def append_BoSC(self, syscall):
        print(len(self.sliding_window))
        if len(self.sliding_window) == self.WINDOW_SIZE:
            bag = self.create_BoSC()
            self.sliding_window = self.sliding_window[1:]
            self.sliding_window.append(syscall)
            if not self.DB_CONNECTION.check_existence(bag):
                return 'Anomaly'
            return 'Normal'
        elif len(self.sliding_window) < self.WINDOW_SIZE:
            self.sliding_window.append(syscall)
            return 'Constructing'
        
    def create_learning_db(self):
        df = pd.read_csv('../syscall-trace.csv')
        self.DB_CONNECTION.validate_bosc_table()
        self.load_lookup_table()
        df.columns = df.columns.map(lambda x: x.strip())

        for index, row in df.iterrows():
            if len(self.sliding_window) == self.WINDOW_SIZE:
                bag = self.create_BoSC()
                self.sliding_window = self.sliding_window[1:]
                self.sliding_window.append(row['Syscall'])
                self.DB_CONNECTION.insert_bag__if_nonexistent(bag)
            elif len(self.sliding_window) < self.WINDOW_SIZE:
                self.sliding_window.append(row['Syscall'])


if __name__ == "__main__":
    if len(sys.argv) > 1:
        if sys.argv[1] == "--learn":
            classifier = BoSC_Creator(10)
            classifier.create_learning_db()
    else:
        classifier = BoSC_Creator(10)
        