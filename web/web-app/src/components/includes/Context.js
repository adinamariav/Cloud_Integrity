import React, { Component } from 'react'
import {Redirect} from 'react-router-dom'
import axios from 'axios';

export const DataContext = React.createContext();


export class DataProvider extends Component {

    state = {
        vmName : [],
        analyzeTime: [],
        learnTime: []
    }

    start = () => {
        const {vmName} = this.state;
        try {
            //const { data } = await axios.post('/run/syscalls', {vmName});
            
            if (vmName == "Select VM") {
                alert("Please select a VM first!");
            }
            else {
                alert(`${vmName}`);
            }
        }
        catch (error) {

        }
    }

    analyzeTimeChanged = (value) => {
        var today = new Date();
        var date = value.split(":");

        var year = today.getFullYear()
        var month = today.getMonth()
        var day = today.getDate()
        var start = new Date(year, month, day, date[0], date[1])

        var seconds = Math.abs(start - today) / 1000

        this.setState( { analyzeTime : seconds } )
    }

    learnTimeChanged = (value) => {
        var today = new Date();
        var date = value.split(":");

        var year = today.getFullYear()
        var month = today.getMonth()
        var day = today.getDate()
        var start = new Date(year, month, day, date[0], date[1])

        var seconds = Math.abs(start - today) / 1000

        this.setState( { learnTime : seconds } )
    }

    selectVM = (vm) => {
        this.setState({vmName : vm})
    }

    componentDidMount() {
        this.setState({vmName : "Select VM"})
    }

    render() {
        const {vmName, learnTime, analyzeTime} = this.state;
        const {start, selectVM, learnTimeChanged, analyzeTimeChanged} = this;
        return (
            <div>
                <DataContext.Provider value={{ vmName, learnTime, analyzeTime, start, selectVM, learnTimeChanged, analyzeTimeChanged }}>
                    {this.props.children}
                </DataContext.Provider>
            </div>
        )
    }
}
