import pandas as pd

df = pd.read_parquet("/workspace/data/test-00000-of-00001.parquet")
df = df.sample(10000, replace=False, random_state=123)
df.to_parquet("/workspace/data/test-00000-of-00001.parquet")
