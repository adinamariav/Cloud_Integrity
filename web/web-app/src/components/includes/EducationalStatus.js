import React, { Component } from "react";
import { DataContext } from "./Context";
import { ProgressBar } from 'react-bootstrap';

class EducationalStatus extends Component {
    static contextType = DataContext;

    state = {
        percentage: 0
    }

    calculateCountdown = () => {
        var percentage = this.state.percentage;

        if (percentage === 0) {
            this.context.startEducational()
        }

        const startDate = new Date();
        const endDate = this.context.endDate;
        const time = this.context.educationalTime;
        const raisePercentage = Math.round(100 / (time * 60))
        
        const timeRemaining = endDate.getTime() - startDate.getTime();
    
        if (timeRemaining > 0) {
          percentage += raisePercentage
          this.setState(() => ( { percentage } ), () => {
            this.timer = setTimeout(this.calculateCountdown, 1000);
          });
        } else {
            alert("Operation completed!")
            this.context.stopEducational();
        } 
      };

    render() {

        const isVMStarted = this.context.isVMStarted;
        const isEducationalStarted = this.context.isEducationalStarted;
        const percentage = this.state.percentage

        if (isVMStarted) {
            if (!isEducationalStarted) {
                return(
                    <div className="container-fluid ml-2 row" style={{maxWidth:`95%`}}>
                        <input class="form-control col" type="text" onChange = { value => this.context.educationalTimeChanged(value)} placeholder="Set time(min)"></input>
                        <button type="button col" onClick={() => this.calculateCountdown()} class="btn row shadow" style= {{ marginLeft: `5rem`, width:`15rem`, marginTop: `-0.5rem`, backgroundColor:`#276678`, color:`white`}}>Start analysis</button>
                    </div>
                )
            } else {
                return (
                    <ProgressBar now={percentage} animated style={{maxWidth:`95%`}}/>
                )
            }
        }
        
    }
}

export default EducationalStatus;