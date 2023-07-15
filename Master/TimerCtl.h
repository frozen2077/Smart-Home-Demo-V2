uint32_t  max_value = 0xFFFFFFFF;         // Max value of the int data type (32 bit)

class TimerCtl
{
  public:
    TimerCtl(uint32_t timeset);           
    void setTimeset(uint32_t timeset);    
    uint32_t getTimeset();                
    uint32_t getRestTime();               
    bool isReady();                       
    bool isStopped();                     
    void reset();                       
    void stopTimer();                     

  private:
    uint32_t _timer = 0;
    uint32_t _timeset = 0;
};

TimerCtl::TimerCtl(uint32_t timeset) {
  _timeset = timeset;
  _timer = millis();
}

void TimerCtl::stopTimer() {
  _timeset = max_value;
  _timer = millis();
}

void TimerCtl::setTimeset(uint32_t timeset) {
  _timeset = timeset;
}

uint32_t TimerCtl::getTimeset() {
  return _timeset;
}

uint32_t TimerCtl::getRestTime() {
  bool stopped = _timeset == max_value;
  int32_t rest = stopped ? max_value : _timeset - (millis() - _timer);
  return rest < 0 ? 0 : rest;
}

bool TimerCtl::isReady() {
  if (millis() - _timer >= _timeset) {
    _timer = millis();
    return true;
  }
  return false;
}

bool TimerCtl::isStopped() {
  bool stopped = _timeset == max_value;
  if (stopped) {
    _timer = millis();
  }
  return stopped;
}

void TimerCtl::reset() {
  _timer = millis();
}
