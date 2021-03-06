DSP Funbox - Set of *nix utilities for experimentation and learning about spectral analysis of images.

![dspfun](http://0x09.net/i/g/dspfun.png "8x8 DFT basis")

#Overview

* [spec](spec) - Generate DCT spectrums for viewing or invertible spectrums for editing in a regular image editor.
* [motion](motion) - 3-dimensional frequency-space editor for video.
* [applybasis](applybasis) - Tools for working with basis functions of a variety of 2D transforms.
* [zoom](zoom) - Interpolate images against a cosine basis.

See subprojects for respective READMEs.

# Dependencies
Most tools in this framework depend on [FFTW](http://www.fftw.org), [ImageMagick](http://www.imagemagick.org), and/or [FFmpeg](http://ffmpeg.org).

# Building
Running

	make

From the top level will build each project.

# TODO
Lots and lots of documentation.