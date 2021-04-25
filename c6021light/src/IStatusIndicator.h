#ifndef __ISTATUSINDICATOR_H__
#define __ISTATUSINDICATOR_H__

/*
 * \brief Class IStatusIndicator
 */
class IStatusIndicator {
 public:
  virtual void setError() = 0;
  virtual void clearError() = 0;

  virtual void setCanDbDownload() = 0;
  virtual void clearCanDbDownload() = 0;
};

#endif  // __ISTATUSINDICATOR_H__
