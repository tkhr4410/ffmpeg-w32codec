#!/bin/sh

SRCDIR=${PWD}/ffmpeg/
BUILDDIR=${PWD}/../../build/ffmpeg/

DECODERS="adpcm_ms,adpcm_ima_wav,cinepak,indeo3,indeo5,mp1,mp2,mp3,mpeg1video,mpeg2video,msvideo1,pcm_alaw,pcm_mulaw,pcm_s16le,pcm_u8,vorbis"
DEMUXERS="avi,mp3,mpegps,mpegvideo,ogg,wav"
PARSERS="mpegaudio,mpegvideo"
PROTOCOLS="file"

mkdir -p ${BUILDDIR}
cd ${BUILDDIR}
sh ${SRCDIR}/configure \
	--prefix=${PWD}/out \
	--disable-static \
	--enable-shared \
	--disable-programs \
	--disable-doc \
	--disable-network \
	--disable-everything \
	--enable-decoder=${DECODERS} \
	--enable-demuxer=${DEMUXERS} \
	--enable-parser=${PARSERS} \
	--enable-protocol=${PROTOCOLS} \
	--disable-iconv \
	--cpu=pentiumpro \
	--cc=clang \
	--extra-ldflags=-static \
	--disable-asm \
	--disable-debug
