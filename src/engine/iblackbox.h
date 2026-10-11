#ifndef IBLACKBOX_H
#define IBLACKBOX_H

#define BLACKBOX_INTERFACE_VERSION "BlackBoxVersion001"

class IBlackBox 
{
public:
	virtual void Record(int type, const char *fmt) = 0;
	virtual void SetLimit(int type, unsigned int count) = 0;
	virtual const char *Get(int type, unsigned int index) = 0;
	virtual float Count(float type) = 0;
	virtual void Flush(float type) = 0;

	virtual const char *GetTypeName(float type) = 0;
	virtual int GetTypeCount() = 0;
};

#endif