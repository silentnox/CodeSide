import subprocess
import sys
import time
import os
import os.path
import json
import shutil
import random


def load_json( name ):
    with open(name) as json_file:
        data = json.load(json_file)
    return data


def write_json( name, data ):
    with open(name,'wt+') as json_file:
        json_file.write(json.dumps(data))

def config_set_seed( seed ):
    config = load_json('batch_config.json')
    config['options_preset']['Custom']['seed'] = seed
    write_json('batch_config.json',config)


if len(sys.argv) < 4:
    quit(0)

num = int(sys.argv[1])
clname1 = str(sys.argv[2])
clname2 = str(sys.argv[3])

if len(sys.argv) >= 5:
    team_size = int(sys.argv[4])
else:
    team_size = 1

if len(sys.argv) >= 6:
    map_name = str(sys.argv[5])
else:
    map_name = 'Simple'

replays = True

total1 = 0
total2 = 0

wins1 = 0
wins2 = 0
draws = 0

if os.path.exists('batch_preset.json'):
    config = load_json('batch_preset.json')
    config['options_preset']['Custom']['properties']['team_size'] = team_size
    config['options_preset']['Custom']['properties']['max_tick_count'] = 3600
    config['options_preset']['Custom']['level'] = map_name
else:
    config = {}
    config['options_preset'] = {'Custom': {'level': map_name, 'properties': None}}

config['players'] = []

if clname1.lower() != 'quickstart':
    config['players'].append({'Tcp': {'host':None,'port':31001,'accept_timeout':None,'timeout':None,'token':None}})
else:
    config['players'].append('Quickstart')

if clname2.lower() != 'quickstart':
    config['players'].append({'Tcp': {'host':None,'port':31002,'accept_timeout':None,'timeout':None,'token':None}})
else:
    config['players'].append('Quickstart')

write_json('batch_config.json',config)

try:
    os.remove('batch_matches.txt')
except:
    pass

try:
    shutil.rmtree('replays/batch')
except:
    pass

os.mkdir('replays/batch')

for i in range(0,num):
    print('Starting game ' + str(i+1) + '...')

    runnerArgs = ['aicup2019.exe', '--config', 'batch_config.json', '--save-results', 'results.txt', '--batch-mode' ]

    if replays:
        runnerArgs.append(['--save-replay ', 'replays/batch/' + str(i+1) + '.rep'])

    localrunner = subprocess.Popen(runnerArgs)
    time.sleep(0.1)

    # runstr = '127.0.0.1 31001';

    if clname1.lower() != 'quickstart':
        client1 = subprocess.Popen([clname1, '127.0.0.1', '31001'])

    if clname2.lower() != 'quickstart':
        client2 = subprocess.Popen([clname2, '127.0.0.1', '31002'])

    localrunner.wait()

    data = load_json('results.txt')

    score1 = data['results'][0]
    score2 = data['results'][1]

    if score1 > score2:
        wins1 += 1
        sign = ' >>> '
    elif score1 < score2:
        wins2 += 1
        sign = ' <<< '
    else:
        draws += 1
        sign = ' === '

    total1 += score1
    total2 += score2

    match_res = str(i+1) + ': ' + str(score1)+'('+str(wins1)+')' + sign + str(score2) +'('+str(wins2)+')\n'

    with open('batch_matches.txt','a+') as f:
        f.write(match_res)

    with open('batch_results.txt','w+') as f:
        f.writelines('Player1: ' + clname1 + ' Player2: ' + clname2 +'\n')
        f.writelines('Score: ' + str(total1) + '/' + str(total2) + '\n')
        f.writelines('Wins: ' + str(wins1) + '/' + str(wins2) + ' (' + str(draws) + ')\n')

print( "Wins: (" + str(wins1) + ":" + str(wins2) + ") Draws: " + str(draws) + " Score: " + str(total1) + ":" + str(total2))