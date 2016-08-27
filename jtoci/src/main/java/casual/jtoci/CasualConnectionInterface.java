package casual.jtoci;


public interface CasualConnectionInterface<CasualBuffer> {

    public Reply casualcall(String service, CasualBuffer data) throws CasualException;
    //public Reply casualcall(String service, CasualBuffer data, int flags);

}
