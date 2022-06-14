import React, { Component } from 'react'
import {Redirect} from 'react-router-dom'
import axios from 'axios';
import io from "socket.io-client"

export const DataContext = React.createContext();
export const socket = io.connect(null, {port: 5000, rememberTransport: false});

export class DataProvider extends Component {

    state = {
        vmName : [],
        analyzeTime: [],
        educationalTime: [],
        isEducationalStarted: false,
        isVMStarted: false, 
        endDate: []
    }

    startEducational = () => {
        const { vmName } = this.state;
        const { educationalTime } = this.state;
        try {
            this.setState( { isEducationalStarted : true } )

            if (vmName === "Select VM") {
                alert("Please select a VM first!");
                return
            }

            if (!educationalTime) {
                alert("Please set the time first!");
                return
            }

            socket.emit('startEducational', vmName, educationalTime);

        }
        catch (error) {

        }
    }

    startVM = () => {
        const {vmName} = this.state;

        try {
            if (vmName === "Select VM") {
                alert("Please select a VM first!");
            }
            else {
                socket.emit('startvm', vmName);
                this.setState( { isVMStarted : true } )
            }
        }
        catch (error) {
            alert("Error starting VM");
        }

    }

    stopEducational = () => {
        this.setState( { isEducationalStarted : false } )
    }

    analyzeTimeChanged = (event) => {
        const time = event.target.value;
        const end = new Date();
        const endDate = new Date(end.getTime() + time * 60000);

        this.setState( { analyzeTime : time } )
        this.setState( { endDate : endDate } )
    }

    educationalTimeChanged = (event) => {
        const time = event.target.value;
        const end = new Date();
        const endDate = new Date(end.getTime() + time * 60000);

        this.setState( { educationalTime : time } )
        this.setState( { endDate : endDate } )
    }

    selectVM = (vm) => {
        this.setState({vmName : vm})
    }

    componentDidMount() {
        this.setState({vmName : "Select VM"})
    }

    render() {
        const { vmName, educationalTime, analyzeTime, isAnalysisStarted, isEducationalStarted, isVMStarted, endDate } = this.state;
        const { startEducational, selectVM, startVM, educationalTimeChanged, analyzeTimeChanged, stopEducational } = this;
        return (
            <div>
                <DataContext.Provider value={{ vmName, educationalTime, analyzeTime, socket, isAnalysisStarted, isEducationalStarted, isVMStarted, endDate, startEducational, startVM, selectVM, educationalTimeChanged, analyzeTimeChanged, stopEducational }}>
                    {this.props.children}
                </DataContext.Provider>
            </div>
        )
    }
}
