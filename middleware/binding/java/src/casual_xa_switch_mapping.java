/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.8
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


public class casual_xa_switch_mapping {
  private transient long swigCPtr;
  protected transient boolean swigCMemOwn;

  protected casual_xa_switch_mapping(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(casual_xa_switch_mapping obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if (swigCPtr != 0) {
      if (swigCMemOwn) {
        swigCMemOwn = false;
        xatmi_serverJNI.delete_casual_xa_switch_mapping(swigCPtr);
      }
      swigCPtr = 0;
    }
  }

  public void setKey(String value) {
    xatmi_serverJNI.casual_xa_switch_mapping_key_set(swigCPtr, this, value);
  }

  public String getKey() {
    return xatmi_serverJNI.casual_xa_switch_mapping_key_get(swigCPtr, this);
  }

  public void setXaSwitch(SWIGTYPE_p_xa_switch_t value) {
    xatmi_serverJNI.casual_xa_switch_mapping_xaSwitch_set(swigCPtr, this, SWIGTYPE_p_xa_switch_t.getCPtr(value));
  }

  public SWIGTYPE_p_xa_switch_t getXaSwitch() {
    long cPtr = xatmi_serverJNI.casual_xa_switch_mapping_xaSwitch_get(swigCPtr, this);
    return (cPtr == 0) ? null : new SWIGTYPE_p_xa_switch_t(cPtr, false);
  }

  public casual_xa_switch_mapping() {
    this(xatmi_serverJNI.new_casual_xa_switch_mapping(), true);
  }

}
