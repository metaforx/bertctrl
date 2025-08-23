#ifndef BERT_UTILS_H
#define BERT_UTILS_H

bool isAboveMax(double value, double max);
bool isBelowMin(double value, double min);
float clamp(float value, float min, float max);
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

#endif // BERT_UTILS_H
