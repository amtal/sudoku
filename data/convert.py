#!/usr/bin/python
#
# Python program to convert files from the ...43.45.3.76.36..34...2.34.. Sudoku
# format to the one used in the course project.
import sys


s = sys.argv[1]

w=16

input = open(s, 'r')
output = open(s+'.txt', 'w')
print input, output

for line in input:
	p = 0
	output.write('start' + str(w) + '\n')
	for c in line:
		if c in '123456789':
			output.write(str(p%w) +' '+  str((p-p%w)/w) +' '+ str(c)+'\n')
		if c in 'abcdefg':
			# if it ain't broke, don't fix it:
			for i in range(97,97+7):
				if chr(i)==c:
					output.write(str(p%w) +' '+  str((p-p%w)/w) +' '+ str((i-87))+'\n')
		p+=1
	output.write('end\n')

input.close()
output.close()
