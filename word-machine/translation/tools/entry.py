"""Field split and rebuild for one dictionary entry. Layout: word \n [ipa] \n meaning lines \n ;example \n zh \n syn \n ant"""
def split(text):
    ls = text.split('\n')
    i = 2 if len(ls) > 1 and ls[1].startswith('[') else 1
    j = next((k for k, l in enumerate(ls) if l.startswith(';')), None)
    if j is None:  # irregular entry: keep every line after the headword as meaning
        return {'word': ls[0], 'ipa': ls[1] if i == 2 else '', 'meaning': ls[i:], 'example': None, 'example_zh': '', 'rest': []}
    return {'word': ls[0], 'ipa': ls[1] if i == 2 else '', 'meaning': ls[i:j], 'example': ls[j][1:],
            'example_zh': ls[j + 1], 'rest': ls[j + 2:]}

def join(f, meaning=None, example_vi=None, rest=None):
    head = [f['word']] + ([f['ipa']] if f['ipa'] else [])
    if f['example'] is None:
        return '\n'.join(head + list(f['meaning'] if meaning is None else meaning))
    return '\n'.join(head + list(f['meaning'] if meaning is None else meaning) + [';' + f['example'],
                     f['example_zh'] if example_vi is None else example_vi] + list(f['rest'] if rest is None else rest))

def soft_limit(zh_bytes, tier):
    """Target size in UTF-8 bytes for meaning plus Vietnamese example. Soft: the validator warns, it does not fail."""
    return max(48, int(zh_bytes * (2.0 if tier == 'A' else 1.5)))
