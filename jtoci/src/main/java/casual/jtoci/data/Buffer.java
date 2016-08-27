package casual.jtoci.data;

import java.util.Iterator;

//TODO ? hur ser casualbuffer ut ?
public interface Buffer {

    BufferFieldsTable getFieldsTable();
    void addField(BufferField field);
    Iterator<BufferField> getFieldIterator();





}
