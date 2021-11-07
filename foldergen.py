import os
os.mkdir('test/inc')
for x in xrange(ord('a'),ord('z')+1):
	os.mkdir('test/inc/'+chr(x))
	for y in xrange(ord('a'),ord('z')+1):
		os.mkdir('test/inc/'+chr(x)+'/'+chr(y))
		for z in xrange(ord('a'),ord('z')+1):
			os.mkdir('test/inc/'+chr(x)+'/'+chr(y)+'/'+chr(z))
