#!/usr/bin/python

import collect_pb2
import gzip
import struct
import sys
import zlib


def _decode(reader):
	head = reader.read(4)
	if not head:
		return None
	assert len(head) == 4
	length, = struct.unpack(">l", head)
	assert length > 8
	body = reader.read(length)
	assert len(body) == length
	assert "SYS0" == body[:4]
	cksum, = struct.unpack(">l", body[-4:])
	cksum2 = zlib.adler32(body[:-4])
	assert cksum == cksum2
	message = collect_pb2.SystemInfo()
	#message.ParseFromString(body[4:-4])
	return message


class Decoder:
	def __init__(self, r):
		self.reader = r

	def decode(self):
		return _decode(self.reader)


def decoder(reader):
	d = Decoder(reader)
	return iter(d.decode, None)


def main(argv):
	for filename in argv:
		with gzip.open(filename) as f:
			for message in decoder(f):
				print message.muduo_timestamp


if __name__ == '__main__':
	main(sys.argv[1:])

