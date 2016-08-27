package casual.jtoci.data;


public interface BufferFieldsTable {

    String getFieldName(int fieldId);
    String[] getFieldNames();
    int getFieldId(String name);
    BufferField getField(int fieldId);
    BufferField getField(String name);

}
