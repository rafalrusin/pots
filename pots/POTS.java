package pots;
public class POTS {
    public static class Dump {
        public void visitThread(Thread t, int state) {
            System.out.println("jthread: " + t.getName() + " state:" + state);
        }

        public void visitMethod(String name, int location) {
            System.out.println("jmethod: " + name + " line:" + location);
        }
        public void visitArg(Object arg) {
        }
        public void endMethod() {
        }
        public void endThread() {
        }

        public void completed() {
        }
    }

    public static class POTSException extends Exception {}

    private static Thread thread;

    public static class POTSRunnable implements Runnable {
        public void run() {
            while (true) {
            }
        }
    }

    public static void init() {
        thread = new Thread(new POTSRunnable());
        thread.start();
        System.out.println("init");
    }

    public static void destroy() {
        if (thread != null) {
            thread.interrupt();
            thread=null;
        }
    }
    
    public static Dump newDump() {
        System.out.println("init");
        return new Dump();
    }
}
