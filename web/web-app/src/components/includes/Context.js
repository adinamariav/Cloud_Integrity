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
        learnTime: [],
        sandboxTime: [],
        educationalTime: [],
        isEducationalStarted: false,
        isVMStarted: false, 
        endDate: [],
        isLearnStarted: false,
        isAnalyzeStarted: false,
        isSandboxStarted: false,
        inputSyscalls: []
    }

    setSyscalls = (text) => {
        this.setState( { inputSyscalls : text })
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

    startLearn = () => {
        const { vmName } = this.state;
        const { learnTime } = this.state;

        try {
            this.setState( { isLearnStarted : true } )

            if (vmName === "Select VM") {
                alert("Please select a VM first!");
                return
            }

            if (!learnTime) {
                alert("Please set the time first!");
                return
            }

            socket.emit('startLearn', vmName, learnTime);

        }
        catch (error) {

        }
    }

    startAnalyze = () => {
        alert("aici")
        const { vmName } = this.state;
        const { analyzeTime } = this.state;

        try {
            this.setState( { isAnalyzeStarted : true } )

            if (vmName === "Select VM") {
                alert("Please select a VM first!");
                return
            }

            if (!analyzeTime) {
                alert("Please set the time first!");
                return
            }

            socket.emit('startAnalyze', vmName, analyzeTime);

        }
        catch (error) {
            console.log(error)
        }
    }

    startSandbox = () => {
        const { vmName } = this.state;
        const { sandboxTime } = this.state;

        const inputs = {}
        let syscalls = []

        inputs.syscalls = syscalls

        try {
            this.setState( { isSandboxStarted : true } )

            if (vmName === "Select VM") {
                alert("Please select a VM first!");
                return
            }

            if (!sandboxTime) {
                alert("Please set the time first!");
                return
            }

            var input = this.state.inputSyscalls.trim().split(/\s+/);

            input.forEach(function (item, index) {
                inputs.syscalls.push(item)
            })

            const object = JSON.stringify(inputs)
            console.log(object)


            socket.emit('startSandbox', vmName, sandboxTime, object);

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

    stopLearn = () => {
        this.setState( { isLearnStarted : false } )
    }

    stopAnalyze = () => {
        this.setState( { isAnalyzeStarted : false } )
    }

    stopSandbox = () => {
        this.setState( { isSandboxStarted : false } )
    }

    analyzeTimeChanged = (event) => {
        const time = event.target.value;
        const end = new Date();
        const endDate = new Date(end.getTime() + time * 60000);

        this.setState( { analyzeTime : time } )
        this.setState( { endDate : endDate } )
    }

    learnTimeChanged = (event) => {
        const time = event.target.value;
        const end = new Date();
        const endDate = new Date(end.getTime() + time * 60000);

        this.setState( { learnTime : time } )
        this.setState( { endDate : endDate } )
    }

    educationalTimeChanged = (event) => {
        const time = event.target.value;
        const end = new Date();
        const endDate = new Date(end.getTime() + time * 60000);

        this.setState( { educationalTime : time } )
        this.setState( { endDate : endDate } )
    }

    sandboxTimeChanged = (event) => {
        const time = event.target.value;
        const end = new Date();
        const endDate = new Date(end.getTime() + time * 60000);

        this.setState( { sandboxTime : time } )
        this.setState( { endDate : endDate } )
    }

    selectVM = (vm) => {
        this.setState({vmName : vm})
    }

    componentDidMount() {
        this.setState({vmName : "Select VM"})
    }

    render() {
        const { vmName, educationalTime, analyzeTime, inputSyscalls, isAnalysisStarted, isEducationalStarted, isVMStarted, endDate, learnTime, isLearnStarted, isAnalyzeStarted, isSandboxStarted, sandboxTime } = this.state;
        const { startEducational, startLearn, setSyscalls, stopLearn, learnTimeChanged, selectVM, startVM, startAnalyze, stopAnalyze, educationalTimeChanged, analyzeTimeChanged, stopEducational, startSandbox, stopSandbox, sandboxTimeChanged } = this;
        return (
            <div>
                <DataContext.Provider value={{ vmName, inputSyscalls, setSyscalls, educationalTime, analyzeTime, socket, isAnalysisStarted, isEducationalStarted, isVMStarted, endDate, learnTime, isLearnStarted, isAnalyzeStarted, isSandboxStarted, sandboxTime, startLearn, stopLearn, startAnalyze, stopAnalyze, learnTimeChanged, startEducational, startVM, selectVM, educationalTimeChanged, startSandbox, stopSandbox, sandboxTimeChanged, analyzeTimeChanged, stopEducational }}>
                    {this.props.children}
                </DataContext.Provider>
            </div>
        )
    }
}
