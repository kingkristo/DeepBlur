// Copyright (c) 2011 The Foundry Visionmongers Ltd.  All Rights Reserved.

#include "DDImage/Row.h"
#include "DDImage/Knobs.h"
#include "DDImage/DDMath.h"
#include "DDImage/LookupCurves.h"
#include "DDImage/DeepFilterOp.h"
#include "DDImage/DeepPixelOp.h"
#include "DDImage/Pixel.h"
#include "DDImage/RGB.h"
#include "stdio.h"
#include <iostream>


//names
static const char* CLASS = "DeepBlur";

using namespace DD::Image;

class DeepBlur : public DeepFilterOp
{
	//define vars
	//REMOVE
	bool _negateColour;
	int _size;
	bool _interpolate;
	bool _blurDepth;
	bool _blurOnZaxis;
	int _image_width;
	int _image_height;
	float _discard_threshold;
	bool _do_dof;
	float dof_depth;
	float dof_increment; 

public:
	DeepBlur(Node* node) : DeepFilterOp(node) {
		std::cout << "STARTING DEEP BLUR";

		//instansiate vars
		_negateColour = true;
		_size = 20;
		_discard_threshold = 5;
		_interpolate = true;
		_blurDepth = true;
		_blurOnZaxis = true;
		_image_width = input_format().width();
		_image_height = input_format().height();
		_do_dof = true;
		dof_depth = 20;
		dof_increment = 1;

	}

	const char* node_help() const override
	{
		return "A deep blur by Alex";
	}

	const char* Class() const override {
		return CLASS;
	}

	Op* op() override
	{
		return this;
	}

	//knobs for adjusting stuff
	void knobs(Knob_Callback f) override
	{
		//rewrite these better
		Bool_knob(f, &_blurDepth, "blur_depth", "blur depth");
		Tooltip(f, "Blur depth channels");

		Bool_knob(f, &_interpolate, "inpterpolate_samples", "intrpolate between samples");
		Tooltip(f, "Interpolate between samples");

		Bool_knob(f, &_blurOnZaxis, "blur_on_z", "blur on z axis only");
		Tooltip(f, "blur all samples per pixel to create one new blurred sample per pixel");

		Int_knob(f, &_size, "blur size");
		SetRange(f, 0, 25);
		Tooltip(f, "The blur box size");

		Float_knob(f, &_discard_threshold, "depth threshold to discard");
		SetRange(f, 0, 25);
		Tooltip(f, "If pixels are further back than this they will not factor into blur");

		Bool_knob(f, &_do_dof, "do_dof", "do depth of field");
		Tooltip(f, "active depth of field effect at specified area");

		Float_knob(f, &dof_depth, "depth of field depth");
		SetRange(f, 0, 25);
		Tooltip(f, "If pixels are further back than this they will not factor into blur");

		Float_knob(f, &dof_increment, "depth of field increment");
		SetRange(f, 0, 25);
		Tooltip(f, "decreminent blur size by 1 each time the distance from depth of field poisition is exceeded by this amount");



	}

	int knob_changed(DD::Image::Knob* k) override
	{
		knob("inpterpolate_samples")->enable(_interpolate);
		knob("blur_depth")->enable(_blurDepth);
		knob("blur_on_z")->enable(_blurOnZaxis);
		knob("do_dof")->enable(_do_dof);
		return 1;
	}


	// I added DONT THINK THIS IS NEEDED?
	/*void getDeepRequests(DD::Image::Box box, const DD::Image::ChannelSet& channels, int count, std::vector<RequestData>& requests) override
	{
		std::cout << "getting requests";
		DD::Image::Box newBox;
		newBox = Box(0, 0, 200, 200);
		DeepFilterOp::getDeepRequests(newBox, channels, count, requests);
	}*/

	void _validate(bool for_real) override
	{
		std::cout << "Validating";

		DeepFilterOp::_validate(for_real);
	}


	bool doDeepEngine(DD::Image::Box box, const ChannelSet& channels, DeepOutputPlane& plane) override
	{

		DD::Image::Box::iterator printit = box.begin(), printitEnd = box.end();
		//will show if new box is being used
		//if (printitEnd.x == 200 || (printitEnd.y - printit.y > 1)) {
		std::cout << "NEW BOX, BEGIN: " << printit.x << ", " << printit.y << " END: " << printitEnd.x << ", " << printitEnd.y << "\n" << "\n";
		//}

		//try and call deep engine on new box using input
		DeepOp* in = input0();
		DeepPlane inPlane;
		DD::Image::Box newBox;
		//hardcoded bounding box
		newBox = Box(0, 0, _image_width, _image_height);
		////populate inPlane with input deep image
		//std::cout << "looking at inplane! " << "\n";;
		if (!in->deepEngine(newBox, channels, inPlane)) {
			//std::cout << "deep engine has died" << "\n";;
			return false;
		}

		plane = DeepOutputPlane(channels, box);
		float value = 0;
		for (Box::iterator it = box.begin();
			it != box.end();
			it++) {


			if ((it.x <= _image_width) && (it.y <= _image_height)) {
				//maybe change to be writable
				DeepPixel pixel = inPlane.getPixel(it);
				size_t inPixelSamples = pixel.getSampleCount();
				DeepOutPixel output;

				//add check for zero blur size
				if (!_blurOnZaxis) {

					//For Eeach sample
					for (size_t iSample = 0; iSample < inPixelSamples; ++iSample) {

						//std::cout << "BLURRING....CURR SAMPLE: -> " << iSample << "\n";
						//pixel to use as sample
						DD::Image::Pixel outPixel(channels);
						foreach(z, channels) {
							outPixel[z] = 0;
						}

						int blurCount = 0;

						float pixelDepth = pixel.getUnorderedSample(iSample, Chan_DeepFront);

						bool outsideDepthRange = false;

						//set SIZE HERE

						int blurSize;

						if (!_do_dof) {
							blurSize = _size;
						}
						else {
							//float dof_depth;
							//float dof_increment;
							blurSize = ceil(_size - (abs(pixelDepth - dof_depth) / dof_increment));
						}
						
						if (blurSize >= 1) {

							//CHECK IF DEPTH EXISTS IN DEIRED RANGE//////////////////////////////////////////////////////////////////////
							//_useZMin _useZMax _useCrop _zrange[0] _zrange[1]


								//z == Chan_Z || z == Chan_Alpha || z == Chan_DeepFront || z == Chan_DeepBack

							for (int px = (it.x - _size); px < (it.x + _size); px++) {
								for (int py = (it.y - _size); py < (it.y + _size); py++) {
									//for (int px = (it.x); px < (it.x + 1); px++) {
										//for (int py = (it.y); py < (it.y + 1); py++) {

									if ((px < _image_width) && (py < _image_height) && (py >= 0) && (px >= 0)) {

										//std::cout << "looking to blur at.. " << px << ", " << py << "\n";

										DeepPixel blurPixel = inPlane.getPixel(py, px);
										//DeepPixel blurPixel = inPlane.getPixel(10, 10);

										//choose which sample...ABSTRACT INTO FUNCTION / METHOD////////

										size_t closestSample = 0;
										float closestDepth = 0;
										bool foundDepthMatch = false;

										size_t closestSampleBack = 0;
										float closestDepthBack = 0;

										size_t closestSampleFront = 0;
										float closestDepthFront = 0;

										//use back if in front, front if behind. To start assume the same and use deepfront
										//std::cout << "LOOKING AT NEW PIXEL... " << "\n" << "\n";

										//for images where different pixels have different num of samples
										size_t blurPixelSamples = blurPixel.getSampleCount();

										for (size_t jSample = 0; jSample < blurPixelSamples; ++jSample) {
											float newDepth = blurPixel.getUnorderedSample(jSample, Chan_DeepFront);

											//std::cout << "LOOKING AT NEW SAMPLE: # " << jSample << "\n";

											//for first sample populate front and back
											if (jSample == 0) {

												closestDepthBack = newDepth;
												closestDepthFront = newDepth;
											}

											//sameDepth
											if (newDepth == pixelDepth) {

												closestDepth = newDepth;
												closestSample = jSample;
												foundDepthMatch = true;
												break;
											}

											//BACK IS 10000, FRONT IS 0

											//in front WE WANT THIS TO BE SMALLER
											if ((newDepth < pixelDepth && newDepth > closestDepthFront) || (newDepth < closestDepthFront && closestDepthFront > pixelDepth)) {

												closestSampleFront = jSample;
												closestDepthFront = newDepth;
											}
											//behind

											if ((newDepth > pixelDepth && newDepth < closestDepthBack) || (newDepth > closestDepthBack && closestDepthBack < pixelDepth)) {


												closestSampleBack = jSample;
												closestDepthBack = newDepth;

											}

											//USE THIS IF NO LINEAR INTERPOLATION
											/*if (abs(pixelDepth - newDepth) < abs(pixelDepth - closestDepth)) {
												closestDepth = newDepth;
												closestSample = jSample;
											}*/

										}

										//if nothing to interpolate between
										if (closestSampleFront == closestSampleBack) {
											closestDepth = closestDepthFront;
											closestSample = closestSampleFront;
											foundDepthMatch = true;
										}

										/*if (!foundDepthMatch) {
											std::cout << "foundDepthMatch: " << foundDepthMatch << "\n";
											std::cout << "closestSample: " << closestSample << "\n";
											std::cout << "frontSample: " << closestSampleFront << "\n";
											std::cout << "backSample: " << closestSampleBack << "\n" << "\n";
										}*/


										//////////////DEPTH ASISGNMENT WORKS//////////////

										//might want to  discard pixel if too far away ADDED NEW
										if (abs(closestDepthFront - pixelDepth) < _discard_threshold || abs(closestDepthBack - pixelDepth) < _discard_threshold) {

											foreach(z, channels) {
												//	ADDING DEFAULT TO OUT PIXEL
												//outPixel[z] += blurPixel.getUnorderedSample(closestSample, z);

												if (!((z == Chan_DeepFront || z == Chan_DeepBack) && !_blurDepth)) {

													if (foundDepthMatch) {
														outPixel[z] += blurPixel.getUnorderedSample(closestSample, z);
													}
													else {

														//if no interp
														//outPixel[z] += blurPixel.getUnorderedSample(closestSampleFront, z);

														//inear interpolation
														if (_interpolate) {
															float x0, y0, x1, y1, xp, yp;

															xp = pixelDepth;
															x1 = closestDepthBack;
															x0 = closestDepthFront;
															y1 = blurPixel.getUnorderedSample(closestSampleBack, z);
															y0 = blurPixel.getUnorderedSample(closestSampleFront, z);

															yp = y0 + ((y1 - y0) / (x1 - x0)) * (xp - x0);

															outPixel[z] += yp;
														}
														else {
															//use front if not interpolating
															outPixel[z] += blurPixel.getUnorderedSample(closestSampleFront, z);
														}
													}
												}

											}

											blurCount++;
										}
									}
								}
							}
						}
						else {

}

						//not sure if needed, should help if all surrounding pixels are discarded
						if (blurCount == 0) {

							foreach(z, channels) {
								outPixel[z] = pixel.getUnorderedSample(iSample, z);
								output.push_back(outPixel[z]);
							}
						}
						else {

							foreach(z, channels) {
								if (!((z == Chan_DeepFront || z == Chan_DeepBack) && !_blurDepth) && !(outsideDepthRange)) {
									outPixel[z] = outPixel[z] / blurCount;
								}
								else {
									//add default values if not blurring them eg. not blurring depth or outside range
									outPixel[z] = pixel.getUnorderedSample(iSample, z);
								}

								output.push_back(outPixel[z]);
							}

						}
						////can probably divide by blurcount here
						//foreach(z, channels) {

						//	//add if to filter out depth channels? OR ADD THIS AS A TOGGLE FEATURE
						//	//z == Chan_Z || z == Chan_Alpha || z == Chan_DeepFront || z == Chan_DeepBack

						//	output.push_back(outPixel[z]);
						//}
					}
				}
				else {

					//std::cout << "creating new output pixel: "<< "\n";

					//blur z axis
					DD::Image::Pixel outPixel(channels);
					foreach(z, channels) {
						outPixel[z] = 0;
					}

					//RUN THROUGH EACH CHANNEL AND SET TO ZERO!!!!!!! DO THIS IN THE OTHER PLACE OUTPIXEL IS USED!!
					//MAKE SURE THAT A COPY OF THE PIXEL IS WHATS BEING PUSHED TO OUTPUT!

					//foreach(z, channels) {
					//	std::cout << "brand new outpixel channel: " << z << " current value: " << outPixel[z] << "\n";
					//	//std::cout << "adding : " << pixel.getUnorderedSample(iSample, z) << "\n";
					//}

					int blurCount = 0;

					for (size_t iSample = 0; iSample < inPixelSamples; ++iSample) {
						//std::cout << "looking at sample: "<< iSample << "\n";

						foreach(z, channels) {
							//add default values if not blurring them eg. not blurring depth or outside range
							//std::cout << "outpixel sample: " << z << " previous value: "<< outPixel[z] << "\n";
							//std::cout << "adding : " << pixel.getUnorderedSample(iSample, z) << "\n";
							outPixel[z] += pixel.getUnorderedSample(iSample, z);
						}

						blurCount++;
					}

					foreach(z, channels) {
						
						outPixel[z] = outPixel[z] / blurCount;

						output.push_back(outPixel[z]);
					}

				}

				//std::cout << "adding pixel to output plane " << "\n";
				//make sure inputting the correct pixel
				plane.addPixel(output);
			}

		}


		return true;
	}


	static const Description d;
	//static const Description d2;
	//static const Description d3;
};


static Op* build(Node* node) { return new DeepBlur(node); }
const Op::Description DeepBlur::d(::CLASS, "Image/DeepBlur", build);
//const Op::Description DeepBlur::d2("DeepMask", "Image/DeepBlur", build);
//const Op::Description DeepBlur::d3("DeepClip", "Image/DeepBlur", build);

//std::cout << "DEEP ENGINE, ";


		/*DD::Image::Box testBox = inPlane.box();

		DD::Image::Box::iterator printBegin = testBox.begin(), printEnd = testBox.end();
		std::cout << "INPUT BOX, BEGIN: " << printBegin.x << ", " << printBegin.y << " END: " << printEnd.x << ", " << printEnd.y << "\n" << "\n";
*/
////simple box blur that blurs each sample layer indpenednantly

////check for no input
//if (!input0()) {
//	std::cout << "no input found";
//	return true;
//}

//DeepOp* in = input0();
//DeepPlane inPlane;

////std::cout << "Input found";

////populate inPlane with input deep image
//if (!in->deepEngine(box, channels, inPlane)) {
//	std::cout << "deep engine has died";
//	return false;
//}

////std::cout << "creating outplane";

////create an output plane
////maybe should be inplace?
//DeepInPlaceOutputPlane outPlane(channels, box);
//
////only works for inplace output plane
////outPlane.reserveSamples(inPlane.getTotalSampleCount());

////from 0,0 to box end
////might be x,y x,y
////only for print
//DD::Image::Box::iterator printit = box.begin(), printitEnd = box.end();
//std::cout << "Looking at BOX, BEGIN: " << printit.x << ", " << printit.y << " END: " << printitEnd.x << ", " << printitEnd.y;

//for (DD::Image::Box::iterator it = box.begin(), itEnd = box.end(); it != itEnd; ++it) {

//	const int x = it.x;
//	const int y = it.y;
//	//copy pixel from input at x,y
//	DeepPixel inPixel = inPlane.getPixel(it);
//	size_t inPixelSamples = inPixel.getSampleCount();
//	//writable output pixel
//	//does this mean its the same as outplane or a copy?
//	DeepOutputPixel outputDeepPixel = outPlane.getPixel(it);
//	//for each sample (slice)
//	for (size_t iSample = 0; iSample < inPixelSamples; ++iSample) {
//		//std::cout << "Looking at sample: " << iSample;
//		//for each channel
//		foreach(z, channels) {
//			//std::cout << "Looking at channel: " << z;
//			float value = 0;
//	
//			DeepPixel blurPixel = inPlane.getPixel(x, y);
//			value = blurPixel.getUnorderedSample(iSample, z);

//			//cpp(120): error C2440: 'initializing': cannot convert from 'float' to 'float *'
//			//should be float*?
//			float outData = outputDeepPixel.getWritableUnorderedSample(iSample, z);

//			//should this be *outData?
//			//cannot convert from 'float' to 'float *'
//			outData = value;
//		}
//	}

//	//add new deep pixel to plane should add left to right bottom to top as we want
//	//this will fill in pixels in the same order they are searched?
//	//cannot access private member declared in class 'DD::Image::DeepInPlaceOutputPlane

//	//,maybe not needed
//	//outPlane.addPixel(outputDeepPixel);

//}

//std::cout << "Writing to outplane \n";

//plane = outPlane;