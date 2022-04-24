import pandas as pd

def create_lookup_table():
    LUT = {}

    data = pd.read_csv('../syscall-trace.csv')
    data.columns = data.columns.map(lambda x: x.strip())

    data['Occur'] = data.groupby('Syscall')['Syscall'].transform('count')
    sorted = data.sort_values('Occur', ascending=False)
    unique = sorted.drop_duplicates(subset='Syscall', keep='last')

    df_stripped = pd.DataFrame(unique, columns = ['Syscall', 'Occur'])
    df_stripped.to_csv('../data/lut.csv', index=False)
    print(df_stripped)

if __name__ == "__main__":
    create_lookup_table()