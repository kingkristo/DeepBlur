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
	bool _negateColour;
	int _size;

public:
	DeepBlur(Node* node) : DeepFilterOp(node) {
		std::cout << "STARTING DEEP BLUR";

		//instansiate vars
		_negateColour = true;
		_size = 20;
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

		Bool_knob(f, &_negateColour, "negate_colour", "negate");
		Tooltip(f, "Negate the colour of the deep image");

	}

	int knob_changed(DD::Image::Knob* k) override
	{
		knob("negate_colour")->enable(_negateColour);
		return 1;
	}

	// I added
	void getDeepRequests(DD::Image::Box box, const DD::Image::ChannelSet& channels, int count, std::vector<RequestData>& requests) override
	{
		std::cout << "getting requests";
		DD::Image::Box newBox;
		newBox = Box(0, 0, 200, 200);
		DeepFilterOp::getDeepRequests(newBox, channels, count, requests);
	}

	void _validate(bool for_real) override
	{
		std::cout << "Validating";

		DeepFilterOp::_validate(for_real);
	}


	bool doDeepEngine(DD::Image::Box box, const ChannelSet& channels, DeepOutputPlane& plane) override
	{

		DD::Image::Box::iterator printit = box.begin(), printitEnd = box.end();
		//will show if new box is being used
		if (printitEnd.x == 200 || (printitEnd.y - printit.y > 1)) {
			std::cout << "NEW BOX, BEGIN: " << printit.x << ", " << printit.y << " END: " << printitEnd.x << ", " << printitEnd.y << "\n" << "\n";
		}

		//try and call deep engine on new box using input
		DeepOp* in = input0();
		DeepPlane inPlane;
		DD::Image::Box newBox;
		newBox = Box(0, 0, 200, 200);
		in->deepEngine(newBox, channels, inPlane);

		plane = DeepOutputPlane(channels, box);
		for (Box::iterator it = box.begin();
				it != box.end();
				it++) {


			//std::cout << "CURR POSITION: -> "<< it.x <<", " << it.y <<" <- \n";

			DeepOutPixel pixel;
			foreach(z, channels)
				pixel.push_back(1);
			plane.addPixel(pixel);
		}
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

