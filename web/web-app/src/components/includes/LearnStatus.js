import React, { Component } from "react";
import { DataContext } from "./Context";
import { ProgressBar } from 'react-bootstrap';

class LearnStatus extends Component {
    static contextType = DataContext;

    state = {
        percentage: 0
    }

    calculateCountdownLearn = () => {
        var percentage = this.state.percentage;

        if (percentage === 0) {
            this.context.startLearn()
        }

        const startDate = new Date();
        const endDate = this.context.endDate;
        const time = this.context.learnTime;
        const raisePercentage = 100 / (time * 60)
        
        const timeRemaining = endDate.getTime() - startDate.getTime();
    
        if (timeRemaining > 0) {
          percentage += raisePercentage
          this.setState(() => ( { percentage } ), () => {
            this.timer = setTimeout(this.calculateCountdownLearn, 1000);
          });
        } else {
            this.context.stopLearn();
        } 
      };

    render() {

        const isVMStarted = this.context.isVMStarted;
        const isLearnStarted = this.context.isLearnStarted;
        const isAnalyzeStarted = this.context.isAnalyzeStarted;
        const percentage = this.state.percentage

        if (isVMStarted) {
            if ((!isLearnStarted) && (!isAnalyzeStarted)) {
                return(
                    <div className="container-fluid ml-2 row" style={{maxWidth:`95%`}}>
                        <input class="form-control col" type="text" onChange = { value => this.context.learnTimeChanged(value)} placeholder="Set time(min)"></input>
                        <button type="button col" onClick={() => this.calculateCountdownLearn()} class="btn row shadow" style= {{ marginLeft: `5rem`, width:`15rem`, marginTop: `-0.5rem`, backgroundColor:`#276678`, color:`white`}}>Start training</button>
                    </div>
                )
            } else {
                if (isLearnStarted) {
                    return (
                        <ProgressBar now={percentage} animated style={{maxWidth:`95%`}}/>
                    )
                }
            }
        }
        
    }
}

export default LearnStatus;