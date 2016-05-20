package com.example.frankchan.ledscroll;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.SeekBar;
import android.widget.Toast;
import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.os.AsyncTask;

import com.example.frankchan.ledscroll.R;

import java.io.IOException;
import java.util.UUID;

public class LEDControl extends AppCompatActivity {

    Button btnSend;
    EditText msgField;
    SeekBar speed;
    ImageButton toTwitter;

    String address = null;
    private ProgressDialog progress;
    BluetoothAdapter myBluetooth = null;
    BluetoothSocket btSocket = null;
    private boolean isBtConnected = false;

    //SPP UUID - For bluetooth
    static final UUID myUUID = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent ledIntent = getIntent();
        address = ledIntent.getStringExtra("BT_ADDRESS"); //receive the address of the bluetooth device

        setContentView(R.layout.activity_ledcontrol);

        //instantiate buttons
        btnSend   =  (Button)findViewById(R.id.button2);
        msgField  =  (EditText)findViewById(R.id.editText);
        speed     =  (SeekBar)findViewById(R.id.seekBar);

        new ConnectBT().execute();

        //send message button click listener
        btnSend.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String temp = msgField.getText().toString();
                msgField.setText("");
                sendBTMsg(temp);
            }
        });

        //seek bar handler
        speed.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if(fromUser) {
                    sendBTMsg(progress);
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                //Do nothing
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                //Do nothing
            }
        });
    }

    //send our message in bytes through bluetooth connection
    public void sendBTMsg(String message) {
        if (btSocket!=null) {
            try {
                btSocket.getOutputStream().write(message.getBytes());
            }
            catch (IOException e) {
                message("There was an error in sending.");
            }
        }
    }

    //overloaded function to send speed through bluetooth connection
    public void sendBTMsg(int speed) {
        if (btSocket!=null) {
            try {
                btSocket.getOutputStream().write(speed);
            }
            catch (IOException e) {
                message("There was an error in sending.");
            }
        }
    }

    // Toast message
    private void message(String msg) {
        Toast.makeText(getApplicationContext(), msg ,Toast.LENGTH_LONG).show();
    }

    //helper class to connect to the bluetooth
    private class ConnectBT extends AsyncTask<Void, Void, Void>  // UI thread
    {
        private boolean ConnectSuccess = true; //if it's here, it's almost connected

        @Override
        protected void onPreExecute()
        {
            progress = ProgressDialog.show(LEDControl.this, "Connecting...", "Please wait!!!");  //show a progress dialog
        }

        @Override
        protected Void doInBackground(Void... devices) //while the progress dialog is shown, the connection is done in background
        {
            try
            {
                //create our socket connection
                if (btSocket == null || !isBtConnected)
                {
                    myBluetooth = BluetoothAdapter.getDefaultAdapter();//get the mobile bluetooth device
                    BluetoothDevice dispositivo = myBluetooth.getRemoteDevice(address);//connects to the device's address and checks if it's available
                    btSocket = dispositivo.createInsecureRfcommSocketToServiceRecord(myUUID);//create a RFCOMM (SPP) connection
                    BluetoothAdapter.getDefaultAdapter().cancelDiscovery();
                    btSocket.connect();//start connection
                }
            }
            catch (IOException e)
            {
                ConnectSuccess = false;//if the try failed, you can check the exception here
            }
            return null;
        }
        @Override
        //return a message with the result
        protected void onPostExecute(Void result) //after the doInBackground, it checks if everything went fine
        {
            super.onPostExecute(result);

            if (!ConnectSuccess)
            {
                message("Connection Failed. Is it a SPP Bluetooth? Try again.");
                finish();
            }
            else
            {
                message("Connected.");
                isBtConnected = true;
            }
            progress.dismiss();
        }
    }
}
