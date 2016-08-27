package casual.jtoci.data;


import java.util.Iterator;

public class CasualBuffer implements Buffer {

    @Override
    public BufferFieldsTable getFieldsTable() {
        throw new RuntimeException("Not implemented yet");
    }

    @Override
    public void addField(BufferField field) {
        throw new RuntimeException("Not implemented yet");
    }

    @Override
    public Iterator<BufferField> getFieldIterator() {
        throw new RuntimeException("Not implemented yet");
    }
}
