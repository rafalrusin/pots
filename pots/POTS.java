package pots;
public class POTS {
    public static class Dump {
        public void visitThread(Thread t, int state, long cpuTime) {
            System.out.println("jthread: " + t.getName() + " state:" + state + " cpuTime:" + cpuTime);
        }

        public void visitMethod(Class methodClass, String methodName, int location) {
            System.out.println("jmethod: " + methodClass + " " + methodName+ " line:" + location);
        }
        public void visitArg(Object arg) {
            String argClass = arg != null ? arg.getClass().toString() : "";
            System.out.println("jarg: " + argClass);
        }
        public void visitArg(int arg) {
            System.out.println("jarg: " + arg);
        }
        public void visitArg(long arg) {
            System.out.println("jarg: " + arg);
        }
        public void visitArg(float arg) {
            System.out.println("jarg: " + arg);
        }
        public void visitArg(double arg) {
            System.out.println("jarg: " + arg);
        }
        public void visitMethodEnd() {
            System.out.println("jmethod end");
        }
        public void visitThreadEnd() {
            System.out.println("jthread end");
        }

        public void completed() {
            System.out.println("jcompleted");
        }
    }

    public static class POTSException extends Exception {
        private static final long serialVersionUID = 1L;
    }

    private static Thread thread;

    public static class POTSRunnable implements Runnable {
        public void run() {
            try {
                while (true) {
                    Thread.sleep(1000);
                    System.out.println("poll");
                    try {
                        throw new POTSException();
                    } catch (POTSException e) {
                    }
                }
            } catch (InterruptedException e) {
                System.out.println("exit");
            }
        }
    }

    public static void init() {
        thread = new Thread(new POTSRunnable(), "POTSPoller");
        thread.setDaemon(true);
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
