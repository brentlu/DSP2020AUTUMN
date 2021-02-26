#!/usr/bin/python3

import argparse

def main():
    # parse argument
    parser = argparse.ArgumentParser()
    parser.add_argument('from_path', help = 'from')
    parser.add_argument('to_path', help = 'to')

    args = parser.parse_args()

    from_path = args.from_path
    to_path = args.to_path

    #print('User input:')
    #print('  from path: %s' % (from_path))
    #print('  to path: %s' % (to_path))

    mapping = dict()

    # process the mapping file
    from_file = open(from_path, 'r', encoding = 'big5-hkscs')

    for line in from_file:
        start = False
        for ch in line:
            if start != False:
                start = False

                if ch not in mapping:
                    # init the set object
                    mapping[ch] = {line[0]}
                else:
                    mapping[ch].add(line[0])

            if ch == ' ' or ch == '/':
                start = True

    from_file.close()

    # write to output mapping file
    out = dict()
    to_file = open(to_path, 'w', encoding = 'big5-hkscs')

    for item in mapping:
        to_file.write('%s' % (item))
        for ch in mapping[item]:
            to_file.write(' %s' % (ch))
        to_file.write('\n')

        # need to handle the word with multiple pronunciations
        for ch in mapping[item]:
            if ch not in out:
                to_file.write('%s %s\n' % (ch, ch))
                out[ch] = True

    to_file.close()

    #for item in mapping:
    #    print('%s: %s\n' % (item, mapping[item]))


    return

if __name__ == '__main__':
    main()
