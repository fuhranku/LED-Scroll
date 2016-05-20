package com.example.frankchan.ledscroll;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import android.widget.Button;
import android.widget.ListView;
import java.util.Set;
import java.util.ArrayList;
import android.widget.Toast;
import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.view.View;
import android.widget.TextView;
import android.content.Intent;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;

import com.example.frankchan.ledscroll.R;

public class MainActivity extends AppCompatActivity {

    Button buttonPaired;
    ListView deviceList;
    private BluetoothAdapter myBluetooth = null;
    private Set<BluetoothDevice> pairedDevices;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        buttonPaired = (Button)findViewById(R.id.button);
        buttonPaired.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                getPairedDevicesList(); //method that will be called
            }
        });

        deviceList = (ListView)findViewById(R.id.listView);
        myBluetooth = BluetoothAdapter.getDefaultAdapter();
        if(myBluetooth == null)
        {
            //Show a message that the device has no bluetooth adapter
            Toast.makeText(getApplicationContext(), "Bluetooth Device Not Available", Toast.LENGTH_LONG).show();
            //finish apk
            finish();
        }
        else
        {
            if (!myBluetooth.isEnabled())
            {
                //Ask to the user turn the bluetooth on
                Intent turnBTon = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(turnBTon,1);
            }
        }
    }

    public void getPairedDevicesList() {
        //returns a set paired devices
        pairedDevices = myBluetooth.getBondedDevices();
        ArrayList<String> bluetoothNames = new ArrayList<String>();

        if (pairedDevices.size() > 0) {
            for(BluetoothDevice bt : pairedDevices) {
                //Device's name
                //Device address
                bluetoothNames.add(bt.getName() + "\n" + bt.getAddress());
            }
        }
        else {
            //Send a toast message
            Toast.makeText(getApplicationContext(), "No Paired Bluetooth Devices Found.", Toast.LENGTH_LONG).show();
        }

        ArrayAdapter<String> adapter = new ArrayAdapter<String>(this,android.R.layout.simple_list_item_1, bluetoothNames);
        deviceList.setAdapter(adapter);
        //Set the onClickListener for each device to be listed
        deviceList.setOnItemClickListener(myListClickListener);
    }

    private AdapterView.OnItemClickListener myListClickListener = new AdapterView.OnItemClickListener()
    {
        public void onItemClick (AdapterView<?> av, View v, int arg2, long arg3)
        {
            // Get the device MAC address, the last 17 chars in the View
            String info = ((TextView) v).getText().toString();
            String address = info.substring(info.length() - 17);

            // Make an intent to start next activity
            Intent i = new Intent(MainActivity.this, LEDControl.class);

            //Store the bluetooth address to be retrieved by future activities
            i.putExtra("BT_ADDRESS", address);

            //Move activity screen to manual LED Control
            startActivity(i);
        }
    };
}
