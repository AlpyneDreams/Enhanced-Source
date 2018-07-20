import re

def strip_comments(str):
    return re.sub(r'#.*', '', str)
