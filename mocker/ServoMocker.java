
import java.io.IOException;
import java.io.PrintStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Scanner;

public class ServoMocker{
    public static void main(String args[]) throws Exception{
        ServerSocket server = new ServerSocket(8888);
        boolean flag = true;
        System.out.println("ServoMocker Starting ...");
        while(true){
            final Socket client = server.accept();
            new Thread(new Runnable(){
                @Override
                public void run(){
                    System.out.println("child process Starting ...");
                    boolean runFlag = true;
                    try{
                        Scanner scan = new Scanner(client.getInputStream());
                        PrintStream out = new PrintStream(client.getOutputStream());
                        while(runFlag){
                            if(scan.hasNext()){
                                double[] realVal = new double[6];
                                String str = scan.next();
                                String[] val = str.split(":");
                                for(int i = 0; i < 6; i ++){
                                    // realVal[i] = (double)val[i*8];
                                    System.out.print(val[i] + " ");
                                    // System.out.print(i  + ": ");
                                }
                                System.out.println();
                            } else {
                                runFlag = false;
                            }
                        }
                    } catch(IOException e){
                        e.printStackTrace();
                    }
                    try{
                        client.close();
                        System.out.println("child process close ...");
                    } catch(IOException e){
                        e.printStackTrace();
                    }
                }
            }).start();
        }
        // System.out.println("ServoMocker Close !");
        // server.close();
    }
}
