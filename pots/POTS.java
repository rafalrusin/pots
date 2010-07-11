package pots;

import java.io.FileOutputStream;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

public class POTS {
    public static class Dump {
        private List<Object> tokens = new ArrayList<Object>();
        
        public static class ArgToken {
            private Object obj;

            public ArgToken(Object obj) {
                this.obj = obj;
            }
            
            private static String fetchValue(Object obj) {
                if (obj instanceof String) {
                    return (String) obj;
                } else return "unknown";
            }
            
            public String toString() {
                if (obj == null) return "argument: {},";
                else return "argument: {"
                + "class: " + quote(obj.getClass().getCanonicalName())
                + ", value: " + quote(fetchValue(obj))
                + "},";
            }
        }
        
        public void init() {
            tokens.add("threads:[");
        }
        
        private static String quote(String s) {
            return "\"" + s + "\"";
        }
        
        public void visitThread(Thread t, int state, long cpuTime) {
            System.out.println("jthread: " + t.getName() + " state:" + state + " cpuTime:" + cpuTime);
            tokens.add("thread:{");
            tokens.add("name:" + quote(t.getName()));
            tokens.add(", state:" + quote("" + state));
            tokens.add(", cpuTime:" + quote("" + cpuTime));
            tokens.add(", frames:[");
        }

        public void visitMethod(Class methodClass, String methodName, int location) {
            System.out.println("jmethod: " + methodClass + " " + methodName+ " line:" + location);
            tokens.add("frame: {");
            tokens.add("methodClass: " + quote(methodClass.toString()));
            tokens.add(", methodName: " + quote(methodName.toString()));
            tokens.add(", location: " + quote("" + location));
            tokens.add(", args: [");
        }
        public void visitArg(Object arg) {
            tokens.add(new ArgToken(arg));
        }
        public void visitArg(int arg) {
            System.out.println("jarg: " + arg);
            tokens.add(new ArgToken(arg));
        }
        public void visitArg(long arg) {
            System.out.println("jarg: " + arg);
            tokens.add(new ArgToken(arg));
        }
        public void visitArg(float arg) {
            System.out.println("jarg: " + arg);
            tokens.add(new ArgToken(arg));
        }
        public void visitArg(double arg) {
            System.out.println("jarg: " + arg);
            tokens.add(new ArgToken(arg));
        }
        public void visitMethodEnd() {
            System.out.println("jmethod end");
            tokens.add("]},");
        }
        public void visitThreadEnd() {
            System.out.println("jthread end");
            tokens.add("]},");
        }

        public void completed() {
            tokens.add("]");
            System.out.println("jcompleted");
            
            saveTokens("pots.log");
        }
        
        private void saveTokens(String fileName) {
            StringBuffer buf = new StringBuffer();
            for (Object o : tokens) {
                buf.append(o.toString());
            }
            buf.append("\n");
            try {
                OutputStream out = new FileOutputStream(fileName, true);
                PrintWriter wt = new PrintWriter(out);
                wt.write(buf.toString());
                wt.close();
                out.close();
            } catch (Exception e) {
                e.printStackTrace();
            }
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
        Dump d = new Dump();
        d.init();
        return d;
    }
}
