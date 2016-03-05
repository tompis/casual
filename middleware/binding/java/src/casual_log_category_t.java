/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 3.0.8
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


public final class casual_log_category_t {
  public final static casual_log_category_t c_log_error = new casual_log_category_t("c_log_error");
  public final static casual_log_category_t c_log_warning = new casual_log_category_t("c_log_warning");
  public final static casual_log_category_t c_log_information = new casual_log_category_t("c_log_information");
  public final static casual_log_category_t c_log_debug = new casual_log_category_t("c_log_debug");

  public final int swigValue() {
    return swigValue;
  }

  public String toString() {
    return swigName;
  }

  public static casual_log_category_t swigToEnum(int swigValue) {
    if (swigValue < swigValues.length && swigValue >= 0 && swigValues[swigValue].swigValue == swigValue)
      return swigValues[swigValue];
    for (int i = 0; i < swigValues.length; i++)
      if (swigValues[i].swigValue == swigValue)
        return swigValues[i];
    throw new IllegalArgumentException("No enum " + casual_log_category_t.class + " with value " + swigValue);
  }

  private casual_log_category_t(String swigName) {
    this.swigName = swigName;
    this.swigValue = swigNext++;
  }

  private casual_log_category_t(String swigName, int swigValue) {
    this.swigName = swigName;
    this.swigValue = swigValue;
    swigNext = swigValue+1;
  }

  private casual_log_category_t(String swigName, casual_log_category_t swigEnum) {
    this.swigName = swigName;
    this.swigValue = swigEnum.swigValue;
    swigNext = this.swigValue+1;
  }

  private static casual_log_category_t[] swigValues = { c_log_error, c_log_warning, c_log_information, c_log_debug };
  private static int swigNext = 0;
  private final int swigValue;
  private final String swigName;
}

