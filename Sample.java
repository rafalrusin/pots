public class Sample {
    int x=12;

    static String eval(String a, int b, Sample s) {
        throw new RuntimeException();
        //return a + new Sample().toString();
    }

    public static void main(String[] args) {
        System.out.println("abc" + eval("bb", 10, new Sample()));
    }
}
